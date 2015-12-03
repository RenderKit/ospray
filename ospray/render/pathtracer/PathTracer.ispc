// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "ospray/camera/Camera.ih"

#include "materials/Medium.ih"
#include "materials/Material.ih"
#include "samplers/PrecomputedSampler.ih"

#define PDF_CULLING 0.0f
//#define USE_DGCOLOR

//////////////////////////////////////////////////////////////////
// LightPath

struct LightPath
{
  Ray    ray;                    /*! Last ray in the path. */
  Medium lastMedium;             /*! Medium the last ray travels inside. */
  uniform uint32 depth;          /*! Recursion depth of path. */
  vec3f  throughput;             /*! Determines the fraction of
                                     radiance reaches the pixel along
                                     the path. */
  bool   ignoreVisibleLights;    /*! If the previous shade point used
                                     shadow rays we have to ignore the
                                     emission of geometrical lights to
                                     not double count them. */
  bool   unbent;                 /*! True of the ray path is a straight line. */
};

inline void init_LightPath(LightPath& lp, const Ray &ray)
{
  lp.ray = ray;
  lp.lastMedium = make_Medium_Vacuum();
  lp.depth = 0;
  lp.throughput = make_vec3f(1.f);
  lp.ignoreVisibleLights = false;
  lp.unbent = true;
}

inline void extend_fast(LightPath& lp,
                   const vec3f nextRay_org,
                   const vec3f nextRay_dir,
                   const float nextRay_near,
                   const float nextRay_far,
                   const vec3f weight,
                   const bool ignoreVL)
{
  lp.unbent = lp.unbent & eq(nextRay_dir,lp.ray.dir);
  setRay(lp.ray,nextRay_org,nextRay_dir,nextRay_near,nextRay_far);
  lp.depth = lp.depth+1;
  lp.throughput = mul(lp.throughput,weight);
  lp.ignoreVisibleLights = ignoreVL;
}

//////////////////////////////////////////////////////////////////
// PathTracer

vec3f transparentShadow(const uniform PathTracer* uniform self, vec3f lightContrib, Ray &shadowRay, Medium medium, varying RandomTEA* uniform rng)
{
  int max_depth = self->maxDepth;
  const float org_t_max = shadowRay.t;
  const uniform uint32/*BRDFType*/ specularBTDFs = (uniform uint32)(SPECULAR_TRANSMISSION);

  while (1) {
    traceRay(self->super.model, shadowRay);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(self->super.model, dg, shadowRay,
        DG_MATERIALID|
        DG_NS|DG_NG|DG_FACEFORWARD|DG_NORMALIZE|DG_TEXCOORD|DG_COLOR
        );

    uniform CompositedBRDF brdfs;
    CompositedBRDF__Constructor(&brdfs);

    uniform PathTraceMaterial *m = (uniform PathTraceMaterial*)dg.material;
    vec3f Ns = dg.Ns;
    foreach_unique(mm in m)
      if (mm != NULL)
        mm->shade(mm, shadowRay, medium, dg, brdfs, Ns);

    Sample3f wi = make_Sample3f(make_vec3f(0.0f),0.0f); uint32 type = 0;
    const vec2f s = make_vec2f(0.5f);
    const vec2f ss = RandomTEA__getFloats(rng);
    const vec3f transparency = CompositedBRDF__sample(&brdfs, shadowRay.dir, dg, wi, type, s, ss.x, specularBTDFs);
    lightContrib = lightContrib * transparency;

    /*! Compute simple volumetric effect. */
    if (ne(medium.transmission, make_vec3f(1.f)))
      lightContrib = lightContrib * powf(medium.transmission, shadowRay.t - shadowRay.t0);

    if (reduce_max(lightContrib) <= self->minContribution)
      return lightContrib;

    if (--max_depth <= 0)
      return make_vec3f(0.f);

    /*! Tracking medium if we hit a medium interface. */
    foreach_unique(mm in m)
      if (mm != NULL)
        mm->selectNextMedium(mm, medium);

    shadowRay.t0 = shadowRay.t + self->super.epsilon;
    shadowRay.t = org_t_max;
    shadowRay.primID = -1;
    shadowRay.geomID = -1;
    shadowRay.instID = -1;
  }
}

vec3f PathTraceIntegrator_Li(const uniform PathTracer* uniform self,
                             const vec2f &pixel, // normalized, i.e. in [0..1]
                             LightPath &lightPath,
                             varying RandomTEA* uniform rng)
{
  uniform uint32/*BRDFType*/ directLightingBRDFTypes = (uniform uint32)(DIFFUSE | GLOSSY);
  uniform uint32/*BRDFType*/ giBRDFTypes = (uniform uint32)(ALL);

  vec3f L = make_vec3f(0.f);
  vec3f Lw = make_vec3f(1.f);

  while (true)
  {
    /*! Terminate path if too long or contribution too low. */
    if (lightPath.depth >= self->maxDepth)
      return L;
    if (reduce_max(lightPath.throughput) <= self->minContribution)
      return L;

    /*! Traverse ray. */
    traceRay(self->super.model, lightPath.ray);

    const vec3f wo = neg(lightPath.ray.dir);

    /*! Environment shading when nothing hit. */
    if (noHit(lightPath.ray)) {
      if ((bool)self->backplate & lightPath.unbent) {
        L = L + Lw * get3f(self->backplate, pixel);
      } else if (!lightPath.ignoreVisibleLights)
          for (uniform int i = 0; i < self->numLights; i++) { // TODO: self->num_envLights
            const uniform Light *uniform l = self->lights[i];
            L = L + Lw * l->evalEnv(l, wo);
          }

      return L;
    }

    DifferentialGeometry dg;
    postIntersect(self->super.model, dg, lightPath.ray,
        DG_MATERIALID|
        DG_NS|DG_NG|DG_FACEFORWARD|DG_NORMALIZE|DG_TEXCOORD|DG_COLOR|DG_TANGENTS
        );

    /*! Shade surface. */
    uniform CompositedBRDF brdfs;
    CompositedBRDF__Constructor(&brdfs);
    vec3f Ns = dg.Ns;
#if 1
    uniform PathTraceMaterial*m = (uniform PathTraceMaterial*)dg.material;
    foreach_unique(mm in m)
      if (mm != NULL)
        mm->shade(mm, lightPath.ray, lightPath.lastMedium, dg, brdfs, Ns);
#else
    foreach_unique(geomID in lightPath.ray.geomID) {
      uniform PathTraceMaterial* uniform m
        = (uniform PathTraceMaterial*)(self->super.model->geometry[geomID]->material);
      print("shading %\n",m);
      if (m != NULL) m->shade(m,lightPath.ray, lightPath.lastMedium, dg, brdfs, Ns);
    }
#endif
    dg.Ns = Ns;

#if 0
    // iw: disabled because we dont' have per-geometry lights yet
    /*! Add light emitted by hit area light source. */
    if (!lightPath.ignoreVisibleLights) {
      foreach_unique(geomID in lightPath.ray.geomID) {
        const uniform AreaLight* uniform l = self->super.model->geometry[geomID]->light;
        if (l != NULL)
          // L = add(L, mul(Lw, l->Le(l,dg,wo)));
          L = L + Lw * l->Le(l,dg,wo);
      }
    }
#endif

    /*! Check if any BRDF component uses direct lighting. */
    bool useDirectLighting = brdfs.brdfTypes & directLightingBRDFTypes;

    /*! Direct lighting. Shoot shadow rays to all light sources. */
    if (useDirectLighting)
    {
      uniform int numLights = self->lights ? min(MAX_LIGHTS, self->numLights) : 0;
      for (uniform int i = 0; i < numLights; i++)
      {
        const uniform Light *uniform light = self->lights[i];

        /*! Either use precomputed samples for the light or sample light now. */
        LightSample ls = light->sample(light, dg, RandomTEA__getFloats(rng));

        /*! Ignore zero radiance or illumination from the back. */
        if (reduce_max(ls.radiance) <= 0.0f | ls.pdf <= PDF_CULLING)  //| dot(dg.Ns, ls.direction) <= 1e-8f)
          continue;

        /*! Evaluate BRDF */
        vec3f brdf = CompositedBRDF__eval(&brdfs, wo, dg, ls.direction, directLightingBRDFTypes);
#ifdef USE_DGCOLOR
        brdf = brdf * make_vec3f(dg.color);
#endif
        if (reduce_max(brdf) <= 0.0f)
          continue;

        /*! Test for shadows. */
        Ray shadow_ray;
        setRay(shadow_ray, dg.P, ls.direction,
               //dg.error*
               self->super.epsilon, ls.distance-
               //dg.error*
               self->super.epsilon);
        shadow_ray.time = lightPath.ray.time;

        vec3f unshaded_light_contrib = Lw * ls.radiance * brdf;
        L = L + transparentShadow(self, unshaded_light_contrib, shadow_ray, lightPath.lastMedium, rng);
      }
    }

    /*! Global illumination. Pick one BRDF component and sample it. */
    if (lightPath.depth >= self->maxDepth)
      return L;

    /*! sample brdf */
    Sample3f wi = make_Sample3f(make_vec3f(0.0f),0.0f); uint32 type = 0;
    vec2f s  = RandomTEA__getFloats(rng);
    vec2f ss = RandomTEA__getFloats(rng);
    vec3f c = CompositedBRDF__sample(&brdfs, wo, dg, wi, type, s, ss.x, giBRDFTypes);
#ifdef USE_DGCOLOR
    if ((type & GLOSSY_REFLECTION) == NONE) // only colorize diffuse component
      c = c * make_vec3f(dg.color);
#endif

    /*! Continue only if we hit something valid. */
    if (reduce_max(c) <= 0.0f | wi.pdf <= PDF_CULLING)
      return L;

    /*! Compute simple volumetric effect. */
    const vec3f transmission = lightPath.lastMedium.transmission;
    if (ne(transmission,make_vec3f(1.f)))
      c = c * powf(transmission,lightPath.ray.t);

    /*! Tracking medium if we hit a medium interface. */
    if (type & TRANSMISSION) {
      foreach_unique(uniMat in dg.material) {
        uniform PathTraceMaterial* uniform m = (uniform PathTraceMaterial *)uniMat;
        if (m != NULL)  m->selectNextMedium(m,lightPath.lastMedium);
      }
    }

    /*! Continue the path. */
    extend_fast(lightPath,
                dg.P,wi.v,//dg.error*
                self->super.epsilon,inf,
           c,(type & directLightingBRDFTypes) != NONE);

    Lw = mul(mul(Lw,c),rcp(wi.pdf));
  }
  return L;
}


uniform PTEnvironmentLight* uniform PTAmbientLight__New(const uniform vec3f& L);
typedef uniform PTEnvironmentLight *uniform EnvLightPtr;
typedef uniform PTLight *uniform LightPtr;

inline ScreenSample PathTracer_renderPixel(uniform PathTracer *uniform self,
                                           const uint32 ix,
                                           const uint32 iy)
{
  uniform FrameBuffer *uniform fb = self->super.fb;

  vec3f L = make_vec3f(0.f);
  uniform Camera *uniform camera = self->super.camera;
  ScreenSample screenSample;

  screenSample.sampleID.x = ix;
  screenSample.sampleID.y = iy;

  // init RNG
  RandomTEA rng_state; varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, fb->size.x*iy+ix, fb->accumID);
  const int spp = max(1, self->super.spp);

  for (uniform int s=0; s < spp; s++) {
    screenSample.sampleID.z = fb->accumID*spp + s;

    CameraSample cameraSample;
    const vec2f pixelSample = RandomTEA__getFloats(rng);
    cameraSample.screen.x = (screenSample.sampleID.x + pixelSample.x) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixelSample.y) * fb->rcpSize.y;
    cameraSample.lens     = RandomTEA__getFloats(rng);

    camera->initRay(camera, screenSample.ray, cameraSample);
    const vec2f timeSample = RandomTEA__getFloats(rng);
    screenSample.ray.time = timeSample.x;

    LightPath lightPath;
    init_LightPath(lightPath, screenSample.ray);

    L = L + PathTraceIntegrator_Li(self, cameraSample.screen, lightPath, rng);
  }

  screenSample.alpha = 1.f;
  screenSample.z = inf;
  screenSample.rgb = L * rcpf(spp);
  return screenSample;
}




void *uniform PathTracer_beginFrame(uniform Renderer *uniform _self,
                           uniform FrameBuffer *uniform fb)
{
  _self->fb = fb;
  uniform PathTracer  *uniform self = (uniform PathTracer *uniform)_self;
  return NULL;
}


void PathTracer_renderTileJob(uniform PathTracer *uniform self,
                              uniform Tile &tile,
                              uniform int taskIndex)
{
  uniform FrameBuffer *uniform fb   = self->super.fb;
  uniform int32 spp = self->super.spp;

  const uniform int blocks = fb->accumID > 0 || spp > 0 ?
                               1 : min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = min(begin + RENDERTILE_PIXELS_PER_JOB, TILE_SIZE*TILE_SIZE/blocks);

  for (uint32 i=begin+programIndex;i<end;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i*blocks];
    const uint32 iy = tile.region.lower.y + z_order.ys[i*blocks];
    if (ix >= fb->size.x || iy >= fb->size.y)
      continue;

    ScreenSample screenSample = PathTracer_renderPixel(self, ix, iy);

    for (uniform int p = 0; p < blocks; p++) {
      const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
      setRGBAZ(tile, pixel, screenSample.rgb, screenSample.alpha, screenSample.z);
    }
  }
}

void PathTracer_renderTile(uniform Renderer *uniform _self,
                           void *uniform perFrameData,
                           uniform Tile &tile,
                           uniform int jobID)
{
  uniform PathTracer *uniform self = (uniform PathTracer *uniform)_self;

  PathTracer_renderTileJob(self, tile, jobID);
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void PathTracer_set(void *uniform _self,
                           const uniform int32 maxDepth,
                           const uniform float minContribution,
                           void *uniform backplate,
                           void **uniform lights,
                           const uniform uint32 numLights)
{
  uniform PathTracer *uniform self = (uniform PathTracer *uniform)_self;

  self->maxDepth = maxDepth;
  self->minContribution = minContribution;
  self->backplate = (uniform Texture2D *uniform)backplate;
  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
}

export void* uniform PathTracer_create(void *uniform cppE)
{
  uniform PathTracer *uniform self = uniform new uniform PathTracer;
  Renderer_Constructor(&self->super,cppE);
  self->super.renderTile   = PathTracer_renderTile;
  self->super.beginFrame   = PathTracer_beginFrame;

  PathTracer_set(self, 20, 0.01f, NULL, NULL, 0);

  precomputeZOrder();

  return self;
}
