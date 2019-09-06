// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "PathTracer.ih"
#include "camera/Camera.ih"

#include "render/pathtracer/materials/Medium.ih"
#include "render/pathtracer/materials/Material.ih"
#include "math/random.ih"
#include "fb/LocalFB.ih"
#include "bsdfs/MicrofacetAlbedoTables.ih"

#include "volumes/HenyeyGreenstein.ih"

#define ROULETTE 1
#define MAX_ROULETTE_CONT_PROB 0.95f
#define PDF_CULLING 0.0f

enum PathVertexType
{
  SURFACE,
  VOLUME,
  CAMERA,
  LIGHT,
  ENVIRONMENT
};

struct PathVertex
{
  DifferentialGeometry dg;
  vec3f wi;
  vec3f wo;

  vec3f albedo;

  const varying BSDF* bsdf; // only valid if path vertex is surface vertex
  VolumetricModel* volume;  // only valid if path vertex is volume vertex

  float pdf_w; // the pdf of sampling wo in solid angle measure
  PathVertexType type;
};

inline bool isSmooth(const PathVertex& pathVertex)
{
  if (pathVertex.type == SURFACE && pathVertex.bsdf   && pathVertex.bsdf->type & SCATTERING_SMOOTH) return true;
  if (pathVertex.type == VOLUME  && pathVertex.volume && HenyeyGreenstein_isSmooth(pathVertex.volume->anisotropy)) return true;
  return false;
}

struct PathContext
{
  const PathTracer* uniform context;
  const World* uniform world;
  const varying vec2f* pixel; 
  varying LDSampler* uniform ldSampler;
  varying RandomSampler* uniform randomSampler;
  uniform int numLights;
  uniform uint32 numBounceSampleDims; // BSDF sample (3D), roulette (1D), light samples (N*2D)
};

struct PathState
{
  bool debug;
  bool disableNEE;                          // turn off NEE contribution for debugging purposes
  bool disableFWD;                          // turn off FWD contribution for debugging purposes
  bool homogeneousApprox;                   // handle volume as homogeneous in its bbox for debugging purposes
  bool straightPath;                        // path from camera did not change direction, for alpha and backplate
  bool auxFree;                             // normal & albedo buffer were not yet written to
  uniform uint32 depth;
  uniform uint32 sampleDim;                 // skip: pixel (2D), lens (2D), time (1D)
  vec3f throughput;
  vec3f contribution;
  float time;
  Medium currentMedium;
};

inline float misHeuristic(const PathState& pathState, float pdf1, float pdf2)
{
  if (pathState.disableNEE || pathState.disableFWD)
    return 1.f;

  // power heuristic with beta=2
  const float p = sqr(pdf1) * rcp(sqr(pdf1) + sqr(pdf2));
  // guard against high pdf (including Dirac delta)
  // using the limit when pdf1 approaches inf
  // compare with bit less than sqrt(float_max) (when sqr starts to overflow)
  return pdf1 > 1e17f ? 1.0f : p;
}

// TODO use intersection filters
vec3f transparentShadow(const uniform PathTracer* uniform self,
                        const World *uniform world,
                        vec3f lightContrib,
                        Ray &shadowRay,
                        Medium medium)
{
  uniform int maxDepth = self->super.maxDepth;
  const float tOriginal = shadowRay.t;

  while (1) {
    traceRay(world, shadowRay);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(world, dg, shadowRay,
      DG_MATERIALID |
      DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR);

    uniform PathTraceMaterial *material = (uniform PathTraceMaterial*)dg.material;
    vec3f transparency;
    foreach_unique(m in material)
      if (m != NULL)
        transparency = m->getTransparency(m, dg, shadowRay, medium);

    lightContrib = lightContrib * transparency;

    // compute attenuation with Beer's law
    if (ne(medium.attenuation, 0.f))
      lightContrib = lightContrib
                     * expf(medium.attenuation * (shadowRay.t - shadowRay.t0));

    if (reduce_max(lightContrib) <= self->super.minContribution)
      return lightContrib;

    if (--maxDepth <= 0)
      return make_vec3f(0.f);

    /*! Tracking medium if we hit a medium interface. */
    foreach_unique(m in material)
      if (m != NULL)
        m->selectNextMedium(m, dg, medium);

    shadowRay.t0 = shadowRay.t + dg.epsilon;
    shadowRay.t = tOriginal;
    shadowRay.primID = -1;
    shadowRay.geomID = -1;
    shadowRay.instID = -1;
  }
}

float delta_tracking(
  VolumetricModel *uniform vModel,
  const VolumeInterval& vInterval,
  varying RandomSampler* uniform randomSampler,
  const vec3f& o,
  const vec3f& w,
  float& mu_t,   // sampled extinction coefficint
  vec3f& albedo) // sampled albedo (color)
{
  float mu_max = vModel->densityScale * vModel->maximumDensity;
  float t = vInterval.interval.lower;
  while (true)
  {
    float xi = RandomSampler_getFloat(randomSampler);
    const float dt = - log(1.f - xi) / mu_max;
    t += dt;
    if (t > vInterval.interval.upper)
      break;
    
    xi = RandomSampler_getFloat(randomSampler);
    vec3f p = o + t * w;
    const float sample = vModel->volume->sample(vModel->volume, p);
    mu_t = vModel->densityScale * VolumetricModel_getOpacity(vModel, sample);
    if (xi < mu_t / mu_max)
    {
      albedo = VolumetricModel_getColor(vModel, sample);
      return t;
    }
  }
  return inf;
}

float volumeSampleFreePath(const PathState& pathState,
                           const World *uniform world,
                           Ray &ray,
                           varying LDSampler* uniform ldSampler,
                           varying RandomSampler* uniform randomSampler,
                           VolumetricModel* varying *uniform sampledInstance,
                           float& sampledExtinctionCoefficient,
                           vec3f& sampledAlbedo)
{
  Ray volumeRay;
  volumeRay.org  = ray.org;
  volumeRay.dir  = ray.dir;
  volumeRay.time = ray.time;
  volumeRay.t0   = 0;   // don't clip to ray [t0, t] yet, otherwise we might not get volume intersections 
  volumeRay.t    = inf; // (e.g. if ray(t0) and ray(t) are both inside the volume's bounding box)
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);
  const bool haveVolumeHit = volumeIntervals.numVolumeIntervals > 0;
  float min_dist = inf;
  unmasked { *sampledInstance = NULL; }
  if (haveVolumeHit)
  {
    for (int i = 0; i < volumeIntervals.numVolumeIntervals; ++i) 
    {
      VolumeInterval vInterval = volumeIntervals.intervals[i];
      VolumetricModel *varying instance = volumeIntervals.intervals[i].volumetricModel;
      foreach_unique(inst in instance)
      {
        if (all(__mask == 0)) continue;
        vInterval.interval.lower = max(0.f, vInterval.interval.lower);
        vInterval.interval.upper = noHit(ray) ? vInterval.interval.upper : min(vInterval.interval.upper, ray.t);
        if (hasInterval(vInterval))
        {
          float dist = inf;
          float extinctionCoefficient;
          vec3f albedo;
          if (pathState.homogeneousApprox)
          {
            float density = inst->densityScale;
            dist = vInterval.interval.lower;
            float xi = RandomSampler_getFloat(randomSampler);
            const float dt = - log(1.f - xi) / density;
            dist += dt;
            if (dist > vInterval.interval.upper)
              dist = inf;
          }
          else 
          {
            Ray transformedVolumeRay;
            transformedVolumeRay.org  = volumeRay.org;
            transformedVolumeRay.dir  = volumeRay.dir;
            transformedVolumeRay.time = volumeRay.time;
            transformedVolumeRay.t0   = 0;   // don't clip to ray [t0, t] yet, otherwise we might not get volume intersections 
            transformedVolumeRay.t    = inf; // (e.g. if ray(t0) and ray(t) are both inside the volume's bounding box)
            transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
            dist = delta_tracking(inst,
                                  vInterval,
                                  randomSampler,
                                  transformedVolumeRay.org,
                                  transformedVolumeRay.dir,
                                  extinctionCoefficient,
                                  albedo);
          }
          if (dist < min_dist && dist < inf) {
            min_dist = dist;
            *sampledInstance = inst;
            sampledExtinctionCoefficient = extinctionCoefficient;
            sampledAlbedo = albedo;
          }
        }
      }
    }
  }
  return min_dist;
}

float volumeTransmittance(const PathState& pathState,
                          const World *uniform world,
                          Ray &ray,
                          varying LDSampler* uniform ldSampler,
                          varying RandomSampler* uniform randomSampler)
{
  Ray volumeRay;
  volumeRay.org  = ray.org;
  volumeRay.dir  = ray.dir;
  volumeRay.time = ray.time;
  volumeRay.t0   = 0;   // don't clip to ray [t0, t] yet, otherwise we might not get volume intersections 
  volumeRay.t    = inf; // (e.g. if ray(t0) and ray(t) are both inside the volume's bounding box)
  VolumeIntervals volumeIntervals;
  traceVolumeRay(world, volumeRay, volumeIntervals);
  const bool haveVolumeHit = volumeIntervals.numVolumeIntervals > 0;

  float transmittance = 1.f;
  if (haveVolumeHit)
  {
    for (int i = 0; i < volumeIntervals.numVolumeIntervals; ++i) 
    {
      VolumeInterval vInterval = volumeIntervals.intervals[i];
      VolumetricModel *varying instance = volumeIntervals.intervals[i].volumetricModel;
      foreach_unique(inst in instance)
      {
        vInterval.interval.lower = max(ray.t0, vInterval.interval.lower);
        vInterval.interval.upper = noHit(ray) ? vInterval.interval.upper : min(vInterval.interval.upper, ray.t);
        if (hasInterval(vInterval))
        {
          if (pathState.homogeneousApprox) {
            transmittance = transmittance * exp(-(vInterval.interval.upper - vInterval.interval.lower)*inst->densityScale);
          } else {
            Ray transformedVolumeRay;
            transformedVolumeRay.org  = volumeRay.org;
            transformedVolumeRay.dir  = volumeRay.dir;
            transformedVolumeRay.time = volumeRay.time;
            transformedVolumeRay.t0   = 0;   // don't clip to ray [t0, t] yet, otherwise we might not get volume intersections 
            transformedVolumeRay.t    = inf; // (e.g. if ray(t0) and ray(t) are both inside the volume's bounding box)
            transformRay(transformedVolumeRay, rcp(volumeIntervals.intervals[i].xfm));
            float extinctionCoefficient;
            vec3f albedo;
            const float dist = delta_tracking(
                inst,
                vInterval,
                randomSampler,
                transformedVolumeRay.org,
                transformedVolumeRay.dir,
                extinctionCoefficient,
                albedo);
            transmittance = transmittance * ((dist < vInterval.interval.upper) ? 0.f : 1.f);
          }
        }
      }
    }
  }
  return transmittance;
}

inline vec3f nextEventEstimation(const PathContext& pathContext, 
                                 const PathState& pathState, 
                                 PathVertex& pathVertex)
{
  // direct lighting including shadows and MIS
  vec3f L = make_vec3f(0.f);
  for (uniform int i = 0; i < pathContext.numLights; i++) 
  {
    const uniform Light *uniform light = pathContext.context->lights[i];

    const vec2f s = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim + 4 + i*2);
    Light_SampleRes ls = light->sample(light, pathVertex.dg, s);

    // skip when zero contribution from light
    if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
      continue;

    // evaluate BSDF
    Scattering_EvalRes fe;
    if (pathVertex.type == SURFACE) {
      foreach_unique(f in pathVertex.bsdf) {
        if (f != NULL)
          fe = f->eval(f, pathVertex.wo, ls.dir);
      }
    }
    else {
      foreach_unique(v in pathVertex.volume) {
        if (v != NULL)
          fe = HenyeyGreenstein_eval(v->anisotropy, pathVertex.wo, ls.dir);
      }
    }

    // skip when zero contribution from material
    if (reduce_max(fe.value) <= 0.0f)
      continue;

    // test for shadows
    Ray shadowRay;
    vec3f org = pathVertex.dg.P;
    if (pathVertex.type != VOLUME) {
    if (dot(pathVertex.dg.Ng, ls.dir) < 0.f)
      org = org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
    }
    setRay(shadowRay, org, ls.dir, 0.f, ls.dist, pathState.time);

    const vec3f throughput = pathState.throughput * fe.value;

#if ROULETTE==1
    // Russian roulette adjustment
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float contProb = min(luminance(throughput * rcp(fe.pdf)), MAX_ROULETTE_CONT_PROB);
      fe.pdf *= contProb;
    }
#endif

    const vec3f unshadedLightContrib = throughput * ls.weight;
    const vec3f lightContrib = transparentShadow(pathContext.context, pathContext.world, unshadedLightContrib, shadowRay, pathState.currentMedium);
    if (reduce_max(lightContrib) > 0) 
    {
      const float T = volumeTransmittance(pathState, pathContext.world, shadowRay, pathContext.ldSampler, pathContext.randomSampler);
      if (reduce_max(T) > 0) 
      {
        // we have to use an independent transmittance estimate for MIS to get a correct result
        const float T_mis = pathState.disableFWD || pathState.disableNEE ?  1.f 
                          : volumeTransmittance(pathState, pathContext.world, shadowRay, pathContext.ldSampler, pathContext.randomSampler);
        L = L + T * lightContrib * misHeuristic(pathState, ls.pdf, fe.pdf * T_mis);
      }
    }
  }
  return L;
}

// TODO: get rid of ray in this function
inline vec3f evaluateGeometryLights(const PathState& pathState, 
                                    const PathVertex& lastVertex, 
                                    const PathVertex& pathVertex, 
                                    Ray& ray)
{
  vec3f L = make_vec3f(0.f);
  uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;
  foreach_unique(m in material) {
    if (m != NULL && reduce_max(m->emission) > 0.f) {
      // convert pdf wrt. area to pdf wrt. solid angle
      const float cosd = dot(pathVertex.dg.Ng, ray.dir);
      const float lePdf = pathVertex.dg.areaPDF * sqr(ray.t) / abs(cosd);
      L = L + pathState.throughput * m->emission * misHeuristic(pathState, lastVertex.pdf_w, lePdf);
    }
  }
  return L;
}

// TODO: get rid of ray in this function
inline vec3f evaluateVirtualLights(const PathContext& pathContext, 
                                   const PathState& pathState, 
                                   const PathVertex& lastVertex, 
                                   const PathVertex& pathVertex, 
                                   Ray& ray, 
                                   float maxLightDist)
{
  // add light from *virtual* lights by intersecting them
  vec3f L = make_vec3f(0.f);
  for (uniform int i = pathContext.context->numGeoLights; i < pathContext.numLights; i++) {
    const float minLightDist = distance(lastVertex.dg.P, ray.org); // minDist is not always zero, see above
    const uniform Light *uniform light = pathContext.context->lights[i];
    if (!pathState.straightPath || light->isVisible) {
      // to correctly handle MIS through transparencies the light pdf needs to be calculated wrt. lastVertex.dg
      // however, we only have a valid intersection with the light in [minLightDist, maxLightDist],
      // otherwise light could be added twice
      Light_EvalRes le = light->eval(light, lastVertex.dg, ray.dir, minLightDist, maxLightDist);
      if (reduce_max(le.radiance) > 0.0f) {            
        Ray shadowRay;
        setRay(shadowRay, lastVertex.dg.P, ray.dir, minLightDist, maxLightDist, ray.time);
        const float T = volumeTransmittance(pathState, pathContext.world, shadowRay, pathContext.ldSampler, pathContext.randomSampler);
        L = L + pathState.throughput * le.radiance * misHeuristic(pathState, lastVertex.pdf_w * T, le.pdf);
      }
    }
  }
  return L;
}

inline Scattering_SampleRes sampleDirection(const PathContext& pathContext, 
                                            const PathState& pathState, 
                                            PathVertex& pathVertex)
{
  const vec2f ss = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim);
  const float s  = LDSampler_getFloat (pathContext.ldSampler, pathState.sampleDim+2);
  Scattering_SampleRes fs;
  if (pathVertex.type == SURFACE)
  {
    vec3f Ns = pathVertex.dg.Ns;
    foreach_unique(f in pathVertex.bsdf) {
      if (f != NULL) {
        fs = f->sample(f, pathVertex.wo, ss, s);
        pathVertex.wi  = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
  if (pathVertex.type == VOLUME)
  {
    foreach_unique(v in pathVertex.volume) {
      if (v != NULL) {
        fs = HenyeyGreenstein_sample(v->anisotropy, pathVertex.wo, ss, s);
        pathVertex.wi  = fs.wi;
        pathVertex.pdf_w = fs.pdf;
      }
    }
  }
  return fs;
}

inline void updateAuxilliaryData(PathState& pathState, 
                                 const PathVertex& pathVertex, 
                                 ScreenSample& sample)
{
  sample.albedo = pathVertex.albedo;
  if (pathVertex.type == VOLUME) {
    sample.normal = make_vec3f(0.f);
  }
  else if (pathVertex.type == SURFACE)
  {
    vec3f Ns = pathVertex.dg.Ns;
    foreach_unique(f in pathVertex.bsdf)
      if (f != NULL) { Ns = getN(f); }
    sample.normal = Ns;
  }
  pathState.auxFree = false;
}

inline void postIntersect(const PathContext& pathContext, const PathState& pathState, PathVertex& pathVertex, Ray& ray)
{
  if (pathVertex.type == SURFACE)
  {
    postIntersect(pathContext.world, pathVertex.dg, ray,
                  DG_MATERIALID |
                  DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR | DG_TANGENTS);
  }
  if (pathVertex.type == VOLUME)
  {
      pathVertex.dg.Ng = make_vec3f(0.f);
      pathVertex.dg.Ns = make_vec3f(0.f);
      pathVertex.dg.dPds = make_vec3f(0.f); 
      pathVertex.dg.dPdt = make_vec3f(0.f);
      pathVertex.dg.st = make_vec2f(0);
      pathVertex.dg.color = make_vec4f(0.f);
      pathVertex.dg.primID = RTC_INVALID_GEOMETRY_ID;
      pathVertex.dg.areaPDF = 0.f;
      pathVertex.dg.epsilon = 0.f;
      pathVertex.dg.material = NULL;
  }
}

inline void tracePathSimple(const PathContext& pathContext, PathState& pathState, Ray& ray, ScreenSample& sample)
{
  traceRay(pathContext.world, ray);

  VolumetricModel* varying sampledVolume = NULL;
  float extinctionCoefficient;
  vec3f albedo;
  float freePath = volumeSampleFreePath(pathState, pathContext.world, ray, 
    pathContext.ldSampler, pathContext.randomSampler, 
    &sampledVolume, extinctionCoefficient, albedo);
  if (freePath < inf)
  {
    sample.rgb = make_vec3f(0.f);
  }
  else {
  if (noHit(ray))
    sample.rgb = make_vec3f(1.f)  * albedo;
  else
    sample.rgb = make_vec3f(0.5f) * albedo;
  }
}

inline void tracePath(const PathContext& pathContext, PathState& pathState, Ray& ray, ScreenSample& sample)
{
  PathVertex lastVertex;
  lastVertex.type = CAMERA;
  lastVertex.volume = NULL;
  lastVertex.pdf_w = inf;    // probability density of previous sampled BSDF, for MIS
  lastVertex.dg.P = ray.org; // P and N also used by light eval
  lastVertex.dg.epsilon = calcEpsilon(ray.org, 0.f);
  lastVertex.dg.Ns = ray.dir;
  lastVertex.dg.Ng = ray.dir;

  uniform ShadingContext ctx;
  ShadingContext_Constructor(&ctx);

  float shadowCatcherDist = -inf;
  if (pathContext.context->shadowCatcher)
    shadowCatcherDist = intersectPlane(ray, pathContext.context->shadowCatcherPlane);

  do {
    if (shadowCatcherDist > ray.t0) // valid hit can hide other geometry
      ray.t = min(shadowCatcherDist, ray.t);

    traceRay(pathContext.world, ray);

    PathVertex pathVertex;
    pathVertex.pdf_w = inf;
    pathVertex.volume = NULL;
    if (noHit(ray)) {
      pathVertex.type = ENVIRONMENT;
    } else {
      pathVertex.type = SURFACE;
    }

    // record depth of primary rays
    if (pathState.depth == 0)
      sample.z = ray.t;

    ////////////////////////////////////
    // Shadow Catcher

#if 1
    if (pathState.straightPath) {
      // TODO use MIS as well
      // consider real (flagged) geometries with material and move into
      // light loop (will also handle MIS)
      if (shadowCatcherDist <= ray.t && shadowCatcherDist > ray.t0) {
        // "postIntersect" of shadowCatcher plane
        pathVertex.dg.Ns = pathVertex.dg.Ng = make_vec3f(pathContext.context->shadowCatcherPlane);
        if (dot(ray.dir, pathVertex.dg.Ng) >= 0.f)
          pathVertex.dg.Ns = pathVertex.dg.Ng = neg(pathVertex.dg.Ng);
        const float eps = calcEpsilon(pathVertex.dg.P, ray.dir, shadowCatcherDist);
        pathVertex.dg.P = ray.org + shadowCatcherDist * ray.dir + eps * pathVertex.dg.Ng;

        vec3f unshaded = make_vec3f(0.f); // illumination without occluders
        vec3f shaded = make_vec3f(0.f); // illumination including shadows
        for (uniform int i = 0; i < pathContext.numLights; i++) {
          const uniform Light *uniform light = pathContext.context->lights[i];

          const vec2f s = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim + 4 + i*2);
          Light_SampleRes ls = light->sample(light, pathVertex.dg, s);

          // skip when zero contribution from light
          if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
            continue;

          // evaluate a white diffuse BRDF
          const float brdf = clamp(dot(ls.dir, pathVertex.dg.Ns));// * one_over_pi cancels anyway

          // skip when zero contribution from material
          if (brdf <= 0.0f)
            continue;

          // test for shadows
          Ray shadowRay;
          setRay(shadowRay, pathVertex.dg.P, ls.dir, 0.0f, ls.dist, pathState.time);

          const vec3f unshadedLightContrib = pathState.throughput * ls.weight * brdf;// * misHeuristic(pathState, ls.pdf, brdf);
          unshaded = unshaded + unshadedLightContrib;
          shaded = shaded + transparentShadow(pathContext.context, pathContext.world, unshadedLightContrib, shadowRay, pathState.currentMedium);
        }
        // order of args important to filter NaNs (in case unshaded.X is zero)
        const vec3f ratio = min(pathState.throughput * shaded * rcp(unshaded), pathState.throughput);
#ifdef COLORED_SHADOW_HACK
        const float rm = reduce_min(ratio);
        sample.alpha = 1.0f - rm;
        pathState.contribution = ratio - rm;
#else
        // alpha blend-in black shadow
        sample.alpha = 1.0f - luminance(ratio);
        pathState.contribution = make_vec3f(0.f);
#endif
        break;
      }

      // update dist for potential next intersection (if transparent)
      shadowCatcherDist -= ray.t;
    }
#endif

    pathVertex.wo = neg(ray.dir);
    
    float extinctionCoefficient;
    float freePath = volumeSampleFreePath(pathState, pathContext.world, ray, 
      pathContext.ldSampler, pathContext.randomSampler, 
      &pathVertex.volume, extinctionCoefficient, pathVertex.albedo);
    if (freePath < inf)
    {
      pathVertex.type = VOLUME;
      pathVertex.dg.P = make_vec3f(ray.org + ray.dir * freePath);
      pathState.throughput = pathState.throughput * pathVertex.albedo;
    }

    float maxLightDist;
    // environment shading when nothing hit
    if (pathVertex.type == ENVIRONMENT) {
      maxLightDist = inf; // include envLights (i.e. the ones in infinity)
      if (pathState.straightPath) {
        sample.alpha = 1.0f - luminance(pathState.throughput);
        if ((bool)pathContext.context->backplate) {
          DifferentialGeometry lookup;
          initDgFromTexCoord(lookup, clamp2edge(pathContext.context->backplate, *pathContext.pixel));
          pathState.contribution = pathState.contribution + pathState.throughput * get3f(pathContext.context->backplate, lookup);
          maxLightDist = 1e38; // backplate hides envLights (i.e. the ones at infinity)
        }
      }
    } else {
      // virtual lights are occluded by hit geometry
      // because lastVertex.dg.P can be different from ray.org (when previously sampled a Dirac transmission)
      // we cannot just use ray.t as maxDist
      maxLightDist = distance(lastVertex.dg.P, ray.org + ray.t * ray.dir);
    }
    
    if ((!pathState.disableFWD || pathState.depth == 0) && pathVertex.type != VOLUME) {
      pathState.contribution = pathState.contribution + evaluateVirtualLights(pathContext, pathState, lastVertex, pathVertex, ray, maxLightDist);
    }

    if (pathVertex.type == ENVIRONMENT)
      break;

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    // except if we have geometry lights (which we still need to evaluate for MIS)
    if (pathState.depth >= pathContext.context->super.maxDepth && pathContext.context->numGeoLights == 0)
      break;

    postIntersect(pathContext, pathState, pathVertex, ray);

    if (!pathState.disableFWD && pathVertex.type != VOLUME) {
      pathState.contribution = pathState.contribution + evaluateGeometryLights(pathState, lastVertex, pathVertex, ray);
    }

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    if (pathState.depth >= pathContext.context->super.maxDepth)
      break;

    // shade surface
    ShadingContext_Constructor(&ctx);
    if (pathVertex.type == SURFACE) {
      uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;
      foreach_unique(m in material)
        if (m != NULL)
          pathVertex.bsdf = m->getBSDF(m, &ctx, pathVertex.dg, ray, pathState.currentMedium);
      // terminate path when we don't have any BSDF
      if (!pathVertex.bsdf)
        break;
      pathVertex.albedo = pathVertex.bsdf->albedo;
    }

    // next event estimation
    if (!pathState.disableNEE && isSmooth(pathVertex)) {
      pathState.contribution = pathState.contribution + nextEventEstimation(pathContext, pathState, pathVertex);
    }

    Scattering_SampleRes fs = sampleDirection(pathContext, pathState, pathVertex);
    
    if (pathState.auxFree && (fs.type & SCATTERING_SMOOTH)) {
      updateAuxilliaryData(pathState, pathVertex, sample);
    }

    // terminate path when zero contribution from material
    if (reduce_max(fs.weight) <= 0.0f || fs.pdf <= PDF_CULLING)
      break;

    pathState.throughput = pathState.throughput * fs.weight;

#if ROULETTE==1
    // Russian roulette
    if (pathState.depth >= pathContext.context->rouletteDepth) {
      const float rr = LDSampler_getFloat(pathContext.ldSampler, pathState.sampleDim+3);
      const float contProb = min(luminance(pathState.throughput), MAX_ROULETTE_CONT_PROB);
      if (rr > contProb)
        break;
      pathState.throughput = pathState.throughput * rcp(contProb);
      fs.pdf *= contProb;
    }
#endif

    // compute attenuation with Beer's law
    if (reduce_min(pathState.currentMedium.attenuation) < 0.f)
      pathState.throughput = pathState.throughput * expf(pathState.currentMedium.attenuation * ray.t);

    vec3f ray_org = pathVertex.dg.P;
    if (pathVertex.type == SURFACE)
    {
      // update currentMedium if we hit a medium interface
      // TODO: support nested dielectrics
      if (fs.type & SCATTERING_TRANSMISSION) {
        ray_org = ray_org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
        uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;
        foreach_unique(m in material) {
          if (m != NULL)
            m->selectNextMedium(m, pathVertex.dg, pathState.currentMedium);
        }
      }
    }

    // keep lastBsdfPdf and lastDg when there was a specular transmission
    // to better combine MIS with transparent shadows
    if (fs.type & ~SCATTERING_SPECULAR_TRANSMISSION || pathVertex.type == VOLUME) {
      lastVertex = pathVertex;
    }

    // continue the path
    pathState.straightPath &= eq(ray.dir, fs.wi);
    setRay(ray, ray_org, fs.wi, pathState.time);
    pathState.depth++;
    pathState.sampleDim += pathContext.numBounceSampleDims;
  } while (reduce_max(pathState.throughput) > pathContext.context->super.minContribution);

  sample.rgb = pathState.contribution;
  if (isnan(pathState.contribution.x) || 
      isnan(pathState.contribution.y) || 
      isnan(pathState.contribution.z))
  {
    sample.rgb = make_vec3f(0.f);
    sample.alpha = 1.0f;
  }
}


ScreenSample PathTraceIntegrator_Li(const PathTracer* uniform self,
                                    const World *uniform world,
                                    FrameBuffer *uniform fb,
                                    const uint32 ix,
                                    const uint32 iy,
                                    const vec2f &pixel, // normalized, i.e. in [0..1]
                                    Ray &ray,
                                    varying LDSampler* uniform ldSampler,
                                    varying RandomSampler* uniform randomSampler)
{
  ScreenSample sample;
  sample.alpha = 1.f;
  sample.normal = make_vec3f(0.0f);
  sample.albedo = make_vec3f(0.0f);

  const uniform int numLights = self->lights ? min(MAX_LIGHTS, self->numLights) : 0;
  PathContext pathContext;
  pathContext.context = self;
  pathContext.world = world;
  pathContext.pixel = &pixel;
  pathContext.ldSampler = ldSampler;
  pathContext.randomSampler = randomSampler;
  pathContext.numLights = numLights;
  pathContext.numBounceSampleDims = 4 + numLights*2; // BSDF sample (3D), roulette (1D), light samples (N*2D)

  PathState pathState;
#if 0 // debug MIS
  int c = (int)((pixel.x + pixel.y) * 10.f) % 3;
  if (c == 0) { pathState.disableNEE = false; pathState.disableFWD = false; }
  if (c == 1) { pathState.disableNEE = false; pathState.disableFWD = true; }
  if (c == 2) { pathState.disableNEE = true; pathState.disableFWD = false; }
#else
  pathState.disableNEE = false; 
  pathState.disableFWD = false;
#endif
  pathState.homogeneousApprox = false;
  pathState.straightPath = true; // path from camera did not change direction, for alpha and backplate
  pathState.auxFree = true; // normal & albedo buffer were not yet written to
  pathState.depth = 0;
  pathState.sampleDim = 5; // skip: pixel (2D), lens (2D), time (1D)
  pathState.throughput = make_vec3f(1.f);
  pathState.contribution = make_vec3f(0.f);
  pathState.time = ray.time;
  pathState.currentMedium = make_Medium_vacuum();

  pathState.debug = false;
  //vec2f debug_pixel = make_vec2f(0.45f, 0.45f);
  //if (ix == (int)(debug_pixel.x*fb->size.x) && iy == (int)(debug_pixel.y*fb->size.y))
  //{
  //  pathState.debug = true;
  //}

  tracePath(pathContext, pathState, ray, sample);

  //if (pathState.debug) {
  //  sample.rgb = make_vec3f(0.8f, 0.2f, 0.8f);
  //}

  return sample;
}

inline ScreenSample PathTracer_renderPixel(PathTracer *uniform self,
                                           FrameBuffer *uniform fb,
                                           Camera *uniform camera,
                                           World *uniform world,
                                           const uint32 ix,
                                           const uint32 iy,
                                           const uint32 accumID)
{
  ScreenSample screenSample;
  screenSample.rgb = make_vec3f(0.f);
  screenSample.alpha = 0.f;
  screenSample.z = inf;
  screenSample.normal = make_vec3f(0.f);
  screenSample.albedo = make_vec3f(0.f);

  screenSample.sampleID.x = ix;
  screenSample.sampleID.y = iy;

  LDSampler ldSamplerObj;
  RandomSampler randomSamplerObj;
  varying LDSampler* uniform ldSampler = &ldSamplerObj;
  varying RandomSampler* uniform randomSampler = &randomSamplerObj;
  const int spp = max(1, self->super.spp);

  for (uniform int s=0; s < spp; s++) {
    // init RNG
    const uint32 sampleID = accumID*spp + s;
    screenSample.sampleID.z = sampleID;
    LDSampler_init(ldSampler, fb->size.x*iy+ix, sampleID);
    RandomSampler_init(randomSampler, fb->size.x*iy+ix, sampleID);

    CameraSample cameraSample;
    const vec2f pixelSample = LDSampler_getFloat2(ldSampler, 0);
    cameraSample.screen.x = (screenSample.sampleID.x + pixelSample.x) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixelSample.y) * fb->rcpSize.y;
    cameraSample.lens     = LDSampler_getFloat2(ldSampler, 2);
    cameraSample.time     = LDSampler_getFloat(ldSampler, 4);

    camera->initRay(camera, screenSample.ray, cameraSample);

    ScreenSample sample = PathTraceIntegrator_Li(self, world,
                                                 fb, ix, iy,
                                                 cameraSample.screen,
                                                 screenSample.ray, 
                                                 ldSampler, 
                                                 randomSampler);
    screenSample.rgb = screenSample.rgb + min(sample.rgb, make_vec3f(self->maxRadiance));
    screenSample.alpha = screenSample.alpha + sample.alpha;
    screenSample.z = min(screenSample.z, sample.z);
    screenSample.normal = screenSample.normal + sample.normal;
    screenSample.albedo = screenSample.albedo + sample.albedo;
  }

  const float rspp = rcpf(spp);
  screenSample.rgb = screenSample.rgb * rspp;
  screenSample.alpha = screenSample.alpha * rspp;
  screenSample.normal = screenSample.normal * rspp;
  screenSample.albedo = screenSample.albedo * rspp;

  return screenSample;
}

void PathTracer_renderTileJob(PathTracer *uniform self,
                              FrameBuffer *uniform fb,
                              Camera *uniform camera,
                              World *uniform world,
                              uniform Tile &tile,
                              uniform int taskIndex)
{
  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = min(begin + RENDERTILE_PIXELS_PER_JOB, TILE_SIZE*TILE_SIZE);

  for (uint32 i=begin+programIndex;i<end;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i];
    const uint32 iy = tile.region.lower.y + z_order.ys[i];
    if (ix >= fb->size.x || iy >= fb->size.y)
      continue;

    ScreenSample screenSample =
      PathTracer_renderPixel(self, fb, camera, world, ix, iy, tile.accumID);

    const uint32 pixel = z_order.xs[i] + (z_order.ys[i] * TILE_SIZE);
    setTile(tile, pixel, screenSample);
  }
}

unmasked void PathTracer_renderTile(Renderer *uniform _self,
                                    FrameBuffer *uniform fb,
                                    Camera *uniform camera,
                                    World *uniform world,
                                    void *uniform perFrameData,
                                    uniform Tile &tile,
                                    uniform int jobID)
{
  PathTracer *uniform self = (PathTracer *uniform)_self;
  PathTracer_renderTileJob(self, fb, camera, world, tile, jobID);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void PathTracer_set(void *uniform _self
    , const uniform int32 rouletteDepth
    , const uniform float maxRadiance
    , void *uniform backplate
    , const uniform vec4f &shadowCatcherPlane
    , void **uniform lights
    , const uniform uint32 numLights
    , const uniform uint32 numGeoLights
    )
{
  PathTracer *uniform self = (PathTracer *uniform)_self;

  self->rouletteDepth = rouletteDepth;
  self->maxRadiance = maxRadiance;
  self->backplate = (uniform Texture2D *uniform)backplate;

  uniform vec3f normal = make_vec3f(shadowCatcherPlane);
  const uniform float l = length(normal);
  self->shadowCatcher = l > 0.f;
  const uniform float rl = rcp(l);
  self->shadowCatcherPlane = make_vec4f(normal*rl, shadowCatcherPlane.w*rl);

  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
  self->numGeoLights = numGeoLights;
}

export void* uniform PathTracer_create(void *uniform cppE)
{
  uniform PathTracer *uniform self = uniform new uniform PathTracer;
  Renderer_Constructor(&self->super, cppE);
  self->super.renderTile = PathTracer_renderTile;

  PathTracer_set(self, 5, inf, NULL, make_vec4f(0.f), NULL, 0, 0);

  precomputeMicrofacetAlbedoTables();
  precomputeZOrder();

  return self;
}
