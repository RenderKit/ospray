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

  float pdf_w; // the pdf of sampling wo in solid angle measure
  PathVertexType type;
};

struct PathContext
{
  const PathTracer* uniform context;
  const World* uniform world;
  const varying vec2f* pixel; 
  varying LDSampler* uniform ldSampler;
  varying FastRandom* uniform randomSampler;
  uniform int numLights;
  uniform uint32 numBounceSampleDims; // BSDF sample (3D), roulette (1D), light samples (N*2D)
};

struct PathState
{
  bool straightPath;                        // path from camera did not change direction, for alpha and backplate
  bool auxFree;                             // normal & albedo buffer were not yet written to
  uniform uint32 depth;
  uniform uint32 sampleDim;                 // skip: pixel (2D), lens (2D), time (1D)
};

inline float misHeuristic(float pdf1, float pdf2)
{
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
  varying FastRandom* uniform randomSampler,
  vec3f &o,
  vec3f &w)
{
  float max_density = 1.0f; // TODO: get proper majorant
  float t = vInterval.interval.lower;
  while (true)
  {
    float xi = FastRandom__getFloat(*randomSampler);
    const float dt = - log(1.f - xi) / max_density;
    t += dt;
    if (t > vInterval.interval.upper || !(dt > 0) )
      break;
    
    xi = FastRandom__getFloat(*randomSampler);
    vec3f p = o + t * w;
    const float density = vModel->volume->sample(vModel->volume, p);
    if (xi < density / max_density)
      return t;
  }
  return inf;
}

float volumeSampleFreePath(const World *uniform world,
                           Ray &ray,
                           varying LDSampler* uniform ldSampler,
                           varying FastRandom* uniform randomSampler)
{
  Ray volumeRay;
  volumeRay.org  = ray.org;
  volumeRay.dir  = ray.dir;
  volumeRay.time = ray.time;
  volumeRay.t0   = ray.t0;
  volumeRay.t    = ray.t;
  volumeRay.geomID = RTC_INVALID_GEOMETRY_ID;
  volumeRay.primID = RTC_INVALID_GEOMETRY_ID;
  volumeRay.instID = RTC_INVALID_GEOMETRY_ID;
  VolumeInterval volumeInterval;
  traceVolumeRay(world, volumeRay, volumeInterval);
  const bool haveVolumeHit = hasInterval(volumeInterval);

  float dist = inf;
  if (haveVolumeHit)
  {
    VolumetricModel *varying instance = volumeInterval.volumetricModel;
    //transformRay(volumeRay, volumeInterval.rcp_xfm);
    foreach_unique(inst in instance)
    {
      volumeInterval.interval.lower = max(0.f, volumeInterval.interval.lower);
      volumeInterval.interval.upper = noHit(ray) ? volumeInterval.interval.upper : min(volumeInterval.interval.upper, ray.t);
      dist = delta_tracking(inst,
                            volumeInterval,
                            randomSampler,
                            volumeRay.org,
                            volumeRay.dir);
    }
  }
  return dist;
}

vec3f volumeTransmittance(const World *uniform world,
                          Ray &ray,
                          varying LDSampler* uniform ldSampler,
                          varying FastRandom* uniform randomSampler)
{
  Ray volumeRay;
  volumeRay.org  = ray.org;
  volumeRay.dir  = ray.dir;
  volumeRay.time = ray.time;
  volumeRay.t0   = ray.t0;
  volumeRay.t    = ray.t;
  volumeRay.geomID = RTC_INVALID_GEOMETRY_ID;
  volumeRay.primID = RTC_INVALID_GEOMETRY_ID;
  volumeRay.instID = RTC_INVALID_GEOMETRY_ID;
  VolumeInterval volumeInterval;
  traceVolumeRay(world, volumeRay, volumeInterval);
  const bool haveVolumeHit = hasInterval(volumeInterval);

  float transmittance = 1.f;
  if (haveVolumeHit)
  {
    VolumetricModel *varying instance = volumeInterval.volumetricModel;
    //transformRay(volumeRay, volumeInterval.rcp_xfm);
    foreach_unique(inst in instance)
    {
      volumeInterval.interval.lower = max(0.f, volumeInterval.interval.lower);
      volumeInterval.interval.upper = noHit(ray) ? volumeInterval.interval.upper : min(volumeInterval.interval.upper, ray.t);
      //transmittance = exp(-(volumeInterval.interval.upper - volumeInterval.interval.lower));
      const float dist = delta_tracking(
          inst,
          volumeInterval,
          randomSampler,
          volumeRay.org,
          volumeRay.dir);
      transmittance = (dist < volumeInterval.interval.upper) ? 0.f : 1.f;
    }
  }
  return make_vec3f(transmittance);
}

inline void tracePath(const PathContext& pathContext, PathState& pathState, Ray& ray, ScreenSample& sample)
{
  vec3f L = make_vec3f(0.f); // accumulated radiance
  vec3f Lw = make_vec3f(1.f); // path throughput
  Medium currentMedium = make_Medium_vacuum();

  PathVertex lastVertex;
  lastVertex.type = CAMERA;
  lastVertex.pdf_w = inf;    // probability density of previous sampled BSDF, for MIS
  lastVertex.dg.P = ray.org; // P and N also used by light eval
  lastVertex.dg.epsilon = calcEpsilon(ray.org, 0.f);
  lastVertex.dg.Ns = ray.dir;
  lastVertex.dg.Ng = ray.dir;

  float shadowCatcherDist = -inf;
  if (pathContext.context->shadowCatcher)
    shadowCatcherDist = intersectPlane(ray, pathContext.context->shadowCatcherPlane);

  do {
    if (shadowCatcherDist > ray.t0) // valid hit can hide other geometry
      ray.t = min(shadowCatcherDist, ray.t);

    traceRay(pathContext.world, ray);

    PathVertex pathVertex;
    pathVertex.pdf_w = inf;
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
          setRay(shadowRay, pathVertex.dg.P, ls.dir, 0.0f, ls.dist, ray.time);

          const vec3f unshadedLightContrib = Lw * ls.weight * brdf;// * misHeuristic(ls.pdf, brdf);
          unshaded = unshaded + unshadedLightContrib;
          shaded = shaded + transparentShadow(pathContext.context, pathContext.world, unshadedLightContrib, shadowRay, currentMedium);
        }
        // order of args important to filter NaNs (in case unshaded.X is zero)
        const vec3f ratio = min(Lw * shaded * rcp(unshaded), Lw);
#ifdef COLORED_SHADOW_HACK
        const float rm = reduce_min(ratio);
        sample.alpha = 1.0f - rm;
        L = ratio - rm;
#else
        // alpha blend-in black shadow
        sample.alpha = 1.0f - luminance(ratio);
        L = make_vec3f(0.f);
#endif
        break;
      }

      // update dist for potential next intersection (if transparent)
      shadowCatcherDist -= ray.t;
    }

    const vec3f wo = neg(ray.dir);

    float freePath = volumeSampleFreePath(pathContext.world, ray, pathContext.ldSampler, pathContext.randomSampler);
    if (freePath < inf)
    {
      Lw = Lw * make_vec3f(1.0);
      pathVertex.type = VOLUME;
      pathVertex.dg.P = make_vec3f(ray.org + ray.dir * freePath);
      pathVertex.dg.Ng = make_vec3f(0.f);
      pathVertex.dg.Ns = make_vec3f(0.f);
      pathVertex.dg.dPds = make_vec3f(0.f); 
      pathVertex.dg.dPdt = make_vec3f(0.f);
      pathVertex.dg.st = make_vec2f(0);
      pathVertex.dg.color = make_vec4f(0.f);
      pathVertex.dg.primID = RTC_INVALID_GEOMETRY_ID;
      pathVertex.dg.materialID = -1;
      pathVertex.dg.areaPDF = 0.f;
      pathVertex.dg.material = NULL;
      pathVertex.dg.epsilon = 0.f;
    }

    float maxLightDist;
    // environment shading when nothing hit
    if (pathVertex.type == ENVIRONMENT) {
      maxLightDist = inf; // include envLights (i.e. the ones in infinity)
      if (pathState.straightPath) {
        sample.alpha = 1.0f - luminance(Lw);
        if ((bool)pathContext.context->backplate) {
          DifferentialGeometry lookup;
          initDgFromTexCoord(lookup, clamp2edge(pathContext.context->backplate, *pathContext.pixel));
          L = L + Lw * get3f(pathContext.context->backplate, lookup);
          maxLightDist = 1e38; // backplate hides envLights (i.e. the ones at infinity)
        }
      }
    } else {
      // virtual lights are occluded by hit geometry
      // because lastVertex.dg.P can be different from ray.org (when previously sampled a Dirac transmission)
      // we cannot just use ray.t as maxDist
      maxLightDist = distance(lastVertex.dg.P, ray.org + ray.t * ray.dir);
    }

    if (pathVertex.type != VOLUME)
    {
      // add light from *virtual* lights by intersecting them
      for (uniform int i = pathContext.context->numGeoLights; i < pathContext.numLights; i++) {
        const float minLightDist = distance(lastVertex.dg.P, ray.org); // minDist is not always zero, see above
        const uniform Light *uniform light = pathContext.context->lights[i];
        if (!pathState.straightPath || light->isVisible) {
          // to correctly handle MIS through transparencies the light pdf needs to be calculated wrt. lastVertex.dg
          // however, we only have a valid intersection with the light in [minLightDist, maxLightDist],
          // otherwise light could be added twice
          Light_EvalRes le = light->eval(light, lastVertex.dg, ray.dir, minLightDist, maxLightDist);
          if (reduce_max(le.radiance) > 0.0f)
            L = L + Lw * le.radiance * misHeuristic(lastVertex.pdf_w, le.pdf);
        }
      }
    }

    if (pathVertex.type == ENVIRONMENT)
      break;

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    // except if we have geometry lights (which we still need to evaluate for MIS)
    if (pathState.depth >= pathContext.context->super.maxDepth && pathContext.context->numGeoLights == 0)
      break;

    if (pathVertex.type == SURFACE)
    {
      ////////////////////////////////////
      // handle next surface interaction

      postIntersect(pathContext.world, pathVertex.dg, ray,
                    DG_MATERIALID |
                    DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR | DG_TANGENTS);
      uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)pathVertex.dg.material;

      // evaluate geometry lights
      foreach_unique(m in material) {
        if (m != NULL && reduce_max(m->emission) > 0.f) {
          // convert pdf wrt. area to pdf wrt. solid angle
          const float cosd = dot(pathVertex.dg.Ng, ray.dir);
          const float lePdf = pathVertex.dg.areaPDF * sqr(ray.t) / abs(cosd);
          L = L + Lw * m->emission * misHeuristic(lastVertex.pdf_w, lePdf);
        }
      }

      // terminate after evaluation of lights and before next shading to always have both samples for MIS
      if (pathState.depth >= pathContext.context->super.maxDepth)
        break;

      // shade surface
      uniform ShadingContext ctx;
      ShadingContext_Constructor(&ctx);
      const varying BSDF* bsdf = NULL;

      foreach_unique(m in material)
        if (m != NULL)
          bsdf = m->getBSDF(m, &ctx, pathVertex.dg, ray, currentMedium);

      // terminate path when we don't have any BSDF
      if (!bsdf)
        break;

      // direct lighting including shadows and MIS
      if (bsdf->type & BSDF_SMOOTH) {
        for (uniform int i = 0; i < pathContext.numLights; i++) {
          const uniform Light *uniform light = pathContext.context->lights[i];

          const vec2f s = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim + 4 + i*2);
          Light_SampleRes ls = light->sample(light, pathVertex.dg, s);

          // skip when zero contribution from light
          if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
            continue;

          // evaluate BSDF
          BSDF_EvalRes fe;
          foreach_unique(f in bsdf) {
            if (f != NULL)
              fe = f->eval(f, wo, ls.dir);
          }

          // skip when zero contribution from material
          if (reduce_max(fe.value) <= 0.0f)
            continue;

          // test for shadows
          Ray shadowRay;
          vec3f org = pathVertex.dg.P;
          if (dot(pathVertex.dg.Ng, ls.dir) < 0.f)
            org = org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
          setRay(shadowRay, org, ls.dir, 0.f, ls.dist, ray.time);

          const vec3f nextLw = Lw * fe.value;

          // Russian roulette adjustment
          if (pathState.depth >= pathContext.context->rouletteDepth) {
            const float contProb = min(luminance(nextLw * rcp(fe.pdf)), MAX_ROULETTE_CONT_PROB);
            fe.pdf *= contProb;
          }

          const vec3f unshadedLightContrib = nextLw * ls.weight * misHeuristic(ls.pdf, fe.pdf);
          L = L + transparentShadow(pathContext.context, pathContext.world, unshadedLightContrib, shadowRay, currentMedium)
                * volumeTransmittance(pathContext.world, shadowRay, pathContext.ldSampler, pathContext.randomSampler);
        }
      }

      // sample BSDF
      const vec2f s  = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim);
      const vec2f ss = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim+2); // ss.y used for Russian roulette
      BSDF_SampleRes fs;
      vec3f Ns = pathVertex.dg.Ns;
      foreach_unique(f in bsdf)
        if (f != NULL) {
          fs = f->sample(f, wo, s, ss.x);
          if (f->frame != NULL)
            Ns = getN(f);
        }

      if (pathState.auxFree && (fs.type & BSDF_SMOOTH)) {
        sample.normal = Ns;
        sample.albedo = bsdf->albedo;
        pathState.auxFree = false;
      }

      // terminate path when zero contribution from material
      if (reduce_max(fs.weight) <= 0.0f | fs.pdf <= PDF_CULLING)
        break;

      pathVertex.pdf_w = fs.pdf;
      Lw = Lw * fs.weight;

      // Russian roulette
      if (pathState.depth >= pathContext.context->rouletteDepth) {
        const float contProb = min(luminance(Lw), MAX_ROULETTE_CONT_PROB);
        if (ss.y >= contProb)
          break;
        Lw = Lw * rcp(contProb);
        fs.pdf *= contProb;
      }

      // compute attenuation with Beer's law
      if (reduce_min(currentMedium.attenuation) < 0.f)
        Lw = Lw * expf(currentMedium.attenuation * ray.t);

      // update currentMedium if we hit a medium interface
      // TODO: support nested dielectrics
      vec3f ray_org = pathVertex.dg.P;
      if (fs.type & BSDF_TRANSMISSION) {
        ray_org = ray_org - (2.0f * pathVertex.dg.epsilon) * pathVertex.dg.Ng;
        foreach_unique(m in material) {
          if (m != NULL)
            m->selectNextMedium(m, pathVertex.dg, currentMedium);
        }
      }

      // keep lastBsdfPdf and lastDg when there was a specular transmission
      // to better combine MIS with transparent shadows
      if (fs.type & ~BSDF_SPECULAR_TRANSMISSION) {
        lastVertex = pathVertex;
      }

      // continue the path
      pathState.straightPath &= eq(ray.dir, fs.wi);
      setRay(ray, ray_org, fs.wi, ray.time);
    }
    if (pathVertex.type == VOLUME)
    {
      if (pathState.auxFree) {
        sample.normal = neg(ray.dir);
        sample.albedo = make_vec3f(1.f);
        pathState.auxFree = false;
      }

      // sample isotropic phase function
      const vec2f s  = LDSampler_getFloat2(pathContext.ldSampler, pathState.sampleDim);
      vec3f wo = uniformSampleSphere(1.0, s);
      
      // continue the path
      lastVertex = pathVertex;
      pathState.straightPath &= eq(ray.dir, wo);
      setRay(ray, pathVertex.dg.P, wo, ray.time);
      ray.t = inf;
    }

    pathState.depth++;
    pathState.sampleDim += pathContext.numBounceSampleDims;
  } while (reduce_max(Lw) > pathContext.context->super.minContribution);

  sample.rgb = L;
  if (isnan(L.x) || isnan(L.y) || isnan(L.z)){
    sample.rgb = make_vec3f(0.f);
    sample.alpha = 1.0f;
  }
}


ScreenSample PathTraceIntegrator_Li(const PathTracer* uniform self,
                                    const World *uniform world,
                                    const vec2f &pixel, // normalized, i.e. in [0..1]
                                    Ray &ray,
                                    varying LDSampler* uniform ldSampler,
                                    varying FastRandom* uniform randomSampler)
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
  pathState.straightPath = true; // path from camera did not change direction, for alpha and backplate
  pathState.auxFree = true; // normal & albedo buffer were not yet written to
  pathState.depth = 0;
  pathState.sampleDim = 5; // skip: pixel (2D), lens (2D), time (1D)

  tracePath(pathContext, pathState, ray, sample);

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
  varying LDSampler* uniform ldSampler = &ldSamplerObj;
  const int spp = max(1, self->super.spp);

  for (uniform int s=0; s < spp; s++) {
    // init RNG
    const uint32 sampleID = accumID*spp + s;
    screenSample.sampleID.z = sampleID;
    LDSampler_init(ldSampler, fb->size.x*iy+ix, sampleID);
    FastRandom randomSamplerObj = FastRandom__seed((fb->size.x*iy+ix) + sampleID * fb->size.x*fb->size.y);
    varying FastRandom* uniform randomSampler = &randomSamplerObj;

    CameraSample cameraSample;
    const vec2f pixelSample = LDSampler_getFloat2(ldSampler, 0);
    //const vec2f pixelSample = make_vec2f(0.5f, 0.5f);
    cameraSample.screen.x = (screenSample.sampleID.x + pixelSample.x) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixelSample.y) * fb->rcpSize.y;
    cameraSample.lens     = LDSampler_getFloat2(ldSampler, 2);
    cameraSample.time     = LDSampler_getFloat(ldSampler, 4);

    camera->initRay(camera, screenSample.ray, cameraSample);

    ScreenSample sample = PathTraceIntegrator_Li(self, world,
                                                 cameraSample.screen,
                                                 screenSample.ray, 
                                                 ldSampler, 
                                                 randomSampler);
    screenSample.rgb = screenSample.rgb + min(sample.rgb, make_vec3f(self->maxRadiance));
    //ScreenSample sample;
    //screenSample.rgb = (cameraSample.screen.x < 0.5) 
    //                 ? make_vec3f(FastRandom__getFloat(*randomSampler))
    //                 : make_vec3f(LDSampler_getFloat(ldSampler, 0));
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
