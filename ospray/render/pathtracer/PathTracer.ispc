// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "geometry/Instance.ih"
#include "math/random.ih"
#include "fb/LocalFB.ih"
#include "bsdfs/MicrofacetAlbedoTables.ih"

#define MAX_ROULETTE_CONT_PROB 0.95f
#define PDF_CULLING 0.0f

inline float misHeuristic(float pdf1, float pdf2)
{
  // power heuristic with beta=2
  const float p = sqr(pdf1) * rcp(sqr(pdf1) + sqr(pdf2));
  // guard against high pdf (including Dirac delta)
  // using the limit when pdf1 approaches inf
  // compare with bit less than sqrt(float_max) (when sqr starts to overflow)
  return pdf1 > 1e17f ? 1.0f : p;
}

inline float getEpsilon(const uniform PathTracer* uniform self,
                        const DifferentialGeometry& dg)
{
  return self->super.autoEpsilon ? dg.epsilon : self->super.epsilon;
}

// TODO use intersection filters
vec3f transparentShadow(const uniform PathTracer* uniform self,
                        vec3f lightContrib,
                        Ray &shadowRay,
                        Medium medium)
{
  uniform int maxDepth = self->super.maxDepth;
  const float tOriginal = shadowRay.t;

  while (1) {
    traceRay(self->super.model, shadowRay);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(self->super.model, dg, shadowRay,
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

    shadowRay.t0 = shadowRay.t + getEpsilon(self, dg);
    shadowRay.t = tOriginal;
    shadowRay.primID = -1;
    shadowRay.geomID = -1;
    shadowRay.instID = -1;
  }
}

ScreenSample PathTraceIntegrator_Li(const uniform PathTracer* uniform self,
                                    const vec2f &pixel, // normalized, i.e. in [0..1]
                                    Ray &ray,
                                    varying LDSampler* uniform sampler)
{
  ScreenSample sample;
  sample.alpha = 1.f;
  sample.normal = make_vec3f(0.0f);
  sample.albedo = make_vec3f(0.0f);

  const uniform int numLights = self->lights ? min(MAX_LIGHTS, self->numLights) : 0;
  vec3f L = make_vec3f(0.f); // accumulated radiance
  vec3f Lw = make_vec3f(1.f); // path throughput
  Medium currentMedium = make_Medium_vacuum();
  float lastBsdfPdf = inf; // probability density of previous sampled BSDF, for MIS
  bool straightPath = true; // path from camera did not change direction, for alpha and backplate
  bool auxFree = true; // normal & albedo buffer were not yet written to
  uniform uint32 depth = 0;
  uniform uint32 sampleDim = 5; // skip: pixel (2D), lens (2D), time (1D)
  const uniform uint32 numBounceSampleDims = 4 + numLights*2; // BSDF sample (3D), roulette (1D), light samples (N*2D)
  // geometric configuration of last surface interaction
  DifferentialGeometry lastDg;
  // P and N also used by light eval
  lastDg.P = ray.org;
  lastDg.epsilon = getIntersectionError(ray.org, 0.f);
  lastDg.Ns = ray.dir;
  lastDg.Ng = ray.dir;

  float shadowCatcherDist = -inf;
  if (self->shadowCatcher)
    shadowCatcherDist = intersectPlane(ray, self->shadowCatcherPlane);

  do {
    if (shadowCatcherDist > ray.t0) // valid hit can hide other geometry
      ray.t = min(shadowCatcherDist, ray.t);

    traceRay(self->super.model, ray);

    DifferentialGeometry dg;

    // record depth of primary rays
    if (depth == 0)
      sample.z = ray.t;


    ////////////////////////////////////
    // Shadow Catcher

    if (straightPath) {
      // TODO use MIS as well
      // consider real (flagged) geometries with material and move into
      // light loop (will also handle MIS)
      if (shadowCatcherDist <= ray.t && shadowCatcherDist > ray.t0) {
        // "postIntersect" of shadowCatcher plane
        dg.P = ray.org + shadowCatcherDist * ray.dir;
        dg.epsilon = getIntersectionError(dg.P, shadowCatcherDist);
        dg.Ns = dg.Ng = make_vec3f(self->shadowCatcherPlane);
        if (dot(ray.dir, dg.Ng) >= 0.f)
          dg.Ns = dg.Ng = neg(dg.Ng);

        vec3f unshaded = make_vec3f(0.f); // illumination without occluders
        vec3f shaded = make_vec3f(0.f); // illumination including shadows
        for (uniform int i = 0; i < numLights; i++) {
          const uniform Light *uniform light = self->lights[i];

          const vec2f s = LDSampler_getFloat2(sampler, sampleDim + 4 + i*2);
          Light_SampleRes ls = light->sample(light, dg, s);

          // skip when zero contribution from light
          if (reduce_max(ls.weight) <= 0.0f | ls.pdf <= PDF_CULLING)
            continue;

          // evaluate a white diffuse BRDF
          const float brdf = clamp(dot(ls.dir, dg.Ns));// * one_over_pi cancels anyway

          // skip when zero contribution from material
          if (brdf <= 0.0f)
            continue;

          // test for shadows
          Ray shadowRay;
          setRay(shadowRay, dg.P, ls.dir,
              getEpsilon(self, dg), ls.dist - getEpsilon(self, dg), ray.time);

          const vec3f unshadedLightContrib = Lw * ls.weight * brdf;// * misHeuristic(ls.pdf, brdf);
          unshaded = unshaded + unshadedLightContrib;
          shaded = shaded + transparentShadow(self, unshadedLightContrib, shadowRay, currentMedium);
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

    float  maxLightDist;
    // environment shading when nothing hit
    if (noHit(ray)) {
      maxLightDist = inf; // include envLights (i.e. the ones in infinity)
      if (straightPath) {
        sample.alpha = 1.0f - luminance(Lw);
        if ((bool)self->backplate) {
          DifferentialGeometry lookup;
          initDgFromTexCoord(lookup, clamp2edge(self->backplate, pixel));
          L = L + Lw * get3f(self->backplate, lookup);
          maxLightDist = 1e38; // backplate hides envLights (i.e. the ones at infinity)
        }
      }
    } else {
      // virtual lights are occluded by hit geometry
      // because lastDg.P can be different from ray.org (when previously sampled a Dirac transmission)
      // we cannot just use ray.t as maxDist
      maxLightDist = distance(lastDg.P, ray.org + ray.t * ray.dir);
    }

    // add light from *virtual* lights by intersecting them
    for (uniform int i = self->numGeoLights; i < self->numLights; i++) {
      const float minLightDist = distance(lastDg.P, ray.org); // minDist is not always zero, see above
      const uniform Light *uniform light = self->lights[i];
      if (!straightPath || light->isVisible) {
        // to correctly handle MIS through transparencies the light pdf needs to be calculated wrt. lastDg
        // however, we only have a valid intersection with the light in [minLightDist, maxLightDist],
        // otherwise light could be added twice
        Light_EvalRes le = light->eval(light, lastDg, ray.dir, minLightDist, maxLightDist);
        if (reduce_max(le.radiance) > 0.0f)
          L = L + Lw * le.radiance * misHeuristic(lastBsdfPdf, le.pdf);
      }
    }

    if (noHit(ray))
      break;

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    // except if we have geometry lights (which we still need to evaluate for MIS)
    if (depth >= self->super.maxDepth && self->numGeoLights == 0)
      break;

    ////////////////////////////////////
    // handle next surface interaction

    postIntersect(self->super.model, dg, ray,
                  DG_MATERIALID |
                  DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR | DG_TANGENTS);
    uniform PathTraceMaterial* material = (uniform PathTraceMaterial*)dg.material;

    // evaluate geometry lights
    foreach_unique(m in material)
      if (m != NULL && reduce_max(m->emission) > 0.f) {
        float areaPdf;
        // XXX same hack as in Model.ih; to get areaPDF of hit geometry instance
        if (ray.instID < 0) { // a regular geometry
          areaPdf = self->areaPDF[ray.geomID];
        } else { // an instance
          foreach_unique(instID in ray.instID) {
            Instance *uniform inst = (Instance *uniform)self->super.model->geometry[instID];
            areaPdf = inst->areaPDF[ray.geomID];
          }
        }

        // convert pdf wrt. area to pdf wrt. solid angle
        const float cosd = dot(dg.Ng, ray.dir);
        const float lePdf = areaPdf * sqr(ray.t) / abs(cosd);
        L = L + Lw * m->emission * misHeuristic(lastBsdfPdf, lePdf);
      }

    // terminate after evaluation of lights and before next shading to always have both samples for MIS
    if (depth >= self->super.maxDepth)
      break;

    // shade surface
    uniform ShadingContext ctx;
    ShadingContext_Constructor(&ctx);
    const varying BSDF* bsdf = NULL;

    foreach_unique(m in material)
      if (m != NULL)
        bsdf = m->getBSDF(m, &ctx, dg, ray, currentMedium);

    // terminate path when we don't have any BSDF
    if (!bsdf)
      break;

    // direct lighting including shadows and MIS
    if (bsdf->type & BSDF_SMOOTH) {
      for (uniform int i = 0; i < numLights; i++) {
        const uniform Light *uniform light = self->lights[i];

        const vec2f s = LDSampler_getFloat2(sampler, sampleDim + 4 + i*2);
        Light_SampleRes ls = light->sample(light, dg, s);

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
        setRay(shadowRay, dg.P, ls.dir,
               getEpsilon(self, dg), ls.dist - getEpsilon(self, dg), ray.time);

        const vec3f nextLw = Lw * fe.value;

        // Russian roulette adjustment
        if (depth >= self->rouletteDepth) {
          const float contProb = min(luminance(nextLw * rcp(fe.pdf)), MAX_ROULETTE_CONT_PROB);
          fe.pdf *= contProb;
        }

        const vec3f unshadedLightContrib = nextLw * ls.weight * misHeuristic(ls.pdf, fe.pdf);
        L = L + transparentShadow(self, unshadedLightContrib, shadowRay, currentMedium);
      }
    }

    // sample BSDF
    const vec2f s  = LDSampler_getFloat2(sampler, sampleDim);
    const vec2f ss = LDSampler_getFloat2(sampler, sampleDim+2); // ss.y used for Russian roulette
    BSDF_SampleRes fs;
    vec3f Ns = dg.Ns;
    foreach_unique(f in bsdf)
      if (f != NULL) {
        fs = f->sample(f, wo, s, ss.x);
        if (f->frame != NULL)
          Ns = getN(f);
      }
    if (auxFree && (fs.type & BSDF_SMOOTH)) {
      sample.normal = Ns;
      sample.albedo = bsdf->albedo;
      auxFree = false;
    }

    // terminate path when zero contribution from material
    if (reduce_max(fs.weight) <= 0.0f | fs.pdf <= PDF_CULLING)
      break;

    Lw = Lw * fs.weight;

    // Russian roulette
    if (depth >= self->rouletteDepth) {
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
    if (fs.type & BSDF_TRANSMISSION) {
      foreach_unique(m in material) {
        if (m != NULL)
          m->selectNextMedium(m, dg, currentMedium);
      }
    }

    // keep lastBsdfPdf and lastDg when there was a specular transmission
    // to better combine MIS with transparent shadows
    if (fs.type & ~BSDF_SPECULAR_TRANSMISSION) {
      lastBsdfPdf = fs.pdf;
      lastDg = dg;
    }

    // continue the path
    straightPath &= eq(ray.dir, fs.wi);
    setRay(ray, dg.P, fs.wi, getEpsilon(self, dg), inf, ray.time);
    depth++;
    sampleDim += numBounceSampleDims;
  } while (reduce_max(Lw) > self->super.minContribution);

  sample.rgb = L;
  if (isnan(L.x) || isnan(L.y) || isnan(L.z)){
    sample.rgb = make_vec3f(0.f);
    sample.alpha = 1.0f;
  }
  return sample;
}


inline ScreenSample PathTracer_renderPixel(uniform PathTracer *uniform self,
                                           const uint32 ix,
                                           const uint32 iy,
                                           const uint32 accumID)
{
  uniform FrameBuffer *uniform fb = self->super.fb;

  uniform Camera *uniform camera = self->super.camera;
  ScreenSample screenSample;
  screenSample.rgb = make_vec3f(0.f);
  screenSample.alpha = 0.f;
  screenSample.z = inf;
  screenSample.normal = make_vec3f(0.f);
  screenSample.albedo = make_vec3f(0.f);

  screenSample.sampleID.x = ix;
  screenSample.sampleID.y = iy;

  LDSampler samplerObj;
  varying LDSampler* uniform sampler = &samplerObj;
  const int spp = max(1, self->super.spp);

  for (uniform int s=0; s < spp; s++) {
    // init RNG
    const uint32 sampleID = accumID*spp + s;
    screenSample.sampleID.z = sampleID;
    LDSampler_init(sampler, fb->size.x*iy+ix, sampleID);

    CameraSample cameraSample;
    const vec2f pixelSample = LDSampler_getFloat2(sampler, 0);
    cameraSample.screen.x = (screenSample.sampleID.x + pixelSample.x) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixelSample.y) * fb->rcpSize.y;
    cameraSample.lens     = LDSampler_getFloat2(sampler, 2);
    cameraSample.time     = LDSampler_getFloat(sampler, 4);

    camera->initRay(camera, screenSample.ray, cameraSample);

    ScreenSample sample = PathTraceIntegrator_Li(self, cameraSample.screen,
                                                 screenSample.ray, sampler);
    screenSample.rgb = screenSample.rgb + min(sample.rgb, make_vec3f(self->maxRadiance));
    screenSample.alpha = screenSample.alpha + sample.alpha;
    screenSample.z = min(screenSample.z, sample.z);
    screenSample.normal = screenSample.normal + sample.normal;
    screenSample.albedo = screenSample.albedo + sample.albedo;
  }

  const float rspp = rcpf(spp);
  screenSample.rgb = screenSample.rgb * rspp;
  screenSample.alpha = screenSample.alpha * rspp;
  screenSample.normal = transposed(camera->frame) * (screenSample.normal*rspp);
  screenSample.albedo = screenSample.albedo * rspp;

  return screenSample;
}




void PathTracer_renderTileJob(uniform PathTracer *uniform self,
                              uniform Tile &tile,
                              uniform int taskIndex)
{
  uniform FrameBuffer *uniform fb = self->super.fb;

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = min(begin + RENDERTILE_PIXELS_PER_JOB, TILE_SIZE*TILE_SIZE);

  for (uint32 i=begin+programIndex;i<end;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i];
    const uint32 iy = tile.region.lower.y + z_order.ys[i];
    if (ix >= fb->size.x || iy >= fb->size.y)
      continue;

    ScreenSample screenSample = PathTracer_renderPixel(self, ix, iy, tile.accumID);

    const uint32 pixel = z_order.xs[i] + (z_order.ys[i] * TILE_SIZE);
    setTile(tile, pixel, screenSample);
  }
}

unmasked void PathTracer_renderTile(uniform Renderer *uniform _self,
                           void *uniform perFrameData,
                           uniform Tile &tile,
                           uniform int jobID)
{
  uniform PathTracer *uniform self = (uniform PathTracer *uniform)_self;

  PathTracer_renderTileJob(self, tile, jobID);
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void PathTracer_set(void *uniform _self
    , const uniform int32 rouletteDepth
    , const uniform float maxRadiance
    , void *uniform backplate
    , const uniform vec4f &shadowCatcherPlane
    , void **uniform lights
    , const uniform uint32 numLights
    , const uniform uint32 numGeoLights
    , void *uniform areaPDF
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
  self->areaPDF = (float *uniform)areaPDF;
}

export void* uniform PathTracer_create(void *uniform cppE)
{
  uniform PathTracer *uniform self = uniform new uniform PathTracer;
  Renderer_Constructor(&self->super,cppE);
  self->super.renderTile = PathTracer_renderTile;

  PathTracer_set(self, 5, inf, NULL, make_vec4f(0.f), NULL, 0, 0, NULL);

  precomputeMicrofacetAlbedoTables();
  precomputeZOrder();

  return self;
}
