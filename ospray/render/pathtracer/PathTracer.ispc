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

/*! number of pixels that each job in a parallel rendertile task
    executes together. Must be a multipel of the maximum possible
    programCount (16), and must be smaller than TILE_SIZE (in one
    dimension) */
#define RENDERTILE_PIXELS_PER_JOB 64


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

  while (1) {
    traceRay(self->super.model, shadowRay);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(self->super.model, dg, shadowRay,
        DG_MATERIALID|
        DG_NS|DG_NG|DG_FACEFORWARD|DG_NORMALIZE|DG_TEXCOORD|DG_COLOR
        );

    uniform PathTraceMaterial *m = (uniform PathTraceMaterial*)dg.material;
    vec3f transparency;
    foreach_unique(mm in m)
      if (mm != NULL)
        transparency = mm->getTransparency(mm, dg, shadowRay, medium);

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
                             varying RandomTEA* uniform rng,
                             uint32 &numRays)
{
  uniform uint32/*BSDFType*/ directLightingBSDFTypes = (uniform uint32)(BSDF_DIFFUSE | BSDF_GLOSSY);
  uniform uint32/*BSDFType*/ giBSDFTypes = (uniform uint32)(BSDF_ALL);

  vec3f L = make_vec3f(0.f);
  vec3f Lw = make_vec3f(1.f);

  while (true)
  {
    /*! Terminate path if too long or contribution too low. */
    if (lightPath.depth >= self->maxDepth)
      return L;
    if (reduce_max(lightPath.throughput) <= self->minContribution) // FIXME: why not Lw???
      return L;

    /*! Traverse ray. */
    traceRay(self->super.model, lightPath.ray);
    numRays++;

    const vec3f wo = neg(lightPath.ray.dir);

    /*! Environment shading when nothing hit. */
    if (noHit(lightPath.ray)) {
      if ((bool)self->backplate & lightPath.unbent) {
        const int x = clamp((int)(pixel.x * self->backplate->size.x), 0, (int)self->backplate->size.x-1);
        const int y = clamp((int)(pixel.y * self->backplate->size.y), 0, (int)self->backplate->size.y-1);
        L = self->backplate->get_nearest_varying(self->backplate,x,y);
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
    uniform ShadingContext ctx;
    ShadingContext_Constructor(&ctx);
    const varying BSDF* bsdf = NULL;
#if 1
    uniform PathTraceMaterial* m = (uniform PathTraceMaterial*)dg.material;
    foreach_unique(mm in m)
      if (mm != NULL)
        bsdf = mm->getBSDF(mm, &ctx, dg, lightPath.ray, lightPath.lastMedium);
#else
    foreach_unique(geomID in lightPath.ray.geomID) {
      uniform PathTraceMaterial* uniform m
        = (uniform PathTraceMaterial*)(self->super.model->geometry[geomID]->material);
      print("shading %\n",m);
      if (m != NULL) m->shade(m,lightPath.ray, lightPath.lastMedium, dg, brdfs, Ns);
    }
#endif

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

    /*! Check if any BSDF component uses direct lighting. */
    bool useDirectLighting = bsdf->type & directLightingBSDFTypes;

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

        /*! Evaluate BSDF */
        vec3f bsdfValue;
        float lsDirectionPdf;
        foreach_unique(b in bsdf)
          if (b != NULL)
            bsdfValue = b->eval(b, dg, wo, ls.direction, lsDirectionPdf);

#ifdef USE_DGCOLOR
        bsdfValue = bsdfValue * make_vec3f(dg.color);
#endif
        if (reduce_max(bsdfValue) <= 0.0f)
          continue;

        /*! Test for shadows. */
        numRays++;
        Ray shadow_ray;
        setRay(shadow_ray, dg.P, ls.direction,
               //dg.error*
               self->super.epsilon, ls.distance-
               //dg.error*
               self->super.epsilon);
        shadow_ray.time = lightPath.ray.time;

        vec3f unshaded_light_contrib = Lw * ls.radiance * bsdfValue;
        L = L + transparentShadow(self, unshaded_light_contrib, shadow_ray, lightPath.lastMedium, rng);
      }
    }

    /*! Global illumination. Pick one BSDF component and sample it. */
    if (lightPath.depth >= self->maxDepth)
      return L;

    /*! sample brdf */
    Sample3f wi = make_Sample3f(make_vec3f(0.0f),0.0f); uint32 type = 0;
    vec2f s  = RandomTEA__getFloats(rng);
    vec2f ss = RandomTEA__getFloats(rng); // why vec2f???
    vec3f bsdfWeight;
    foreach_unique(b in bsdf)
      if (b != NULL)
        bsdfWeight = b->sample(b, dg, wo, wi.v, wi.pdf, type, s, ss.x);

#ifdef USE_DGCOLOR
    if ((type & GLOSSY_REFLECTION) == NONE) // only colorize diffuse component
      bsdfWeight = bsdfWeight * make_vec3f(dg.color);
#endif

    /*! Continue only if we hit something valid. */
    if (reduce_max(bsdfWeight) <= 0.0f | wi.pdf <= PDF_CULLING)
      return L;

    /*! Compute simple volumetric effect. */
    const vec3f transmission = lightPath.lastMedium.transmission;
    if (ne(transmission,make_vec3f(1.f)))
      bsdfWeight = bsdfWeight * powf(transmission,lightPath.ray.t);

    /*! Tracking medium if we hit a medium interface. */
    if (type & BSDF_TRANSMISSION) {
      foreach_unique(uniMat in dg.material) {
        uniform PathTraceMaterial* uniform m = (uniform PathTraceMaterial *)uniMat;
        if (m != NULL)  m->selectNextMedium(m,lightPath.lastMedium);
      }
    }

    /*! Continue the path. */
    extend_fast(lightPath,
                dg.P,wi.v,//dg.error*
                self->super.epsilon,inf,
                bsdfWeight*wi.pdf,(type & directLightingBSDFTypes) != BSDF_NONE);

    Lw = mul(Lw,bsdfWeight);
  }
  return L;
}


uniform PTEnvironmentLight* uniform PTAmbientLight__New(const uniform vec3f& L);
typedef uniform PTEnvironmentLight *uniform EnvLightPtr;
typedef uniform PTLight *uniform LightPtr;

inline ScreenSample PathTracer_renderPixel(uniform PathTracer *uniform self,
                                    const uint32 ix,
                                    const uint32 iy,
                                    uint32 &numRays)
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

    L = L + PathTraceIntegrator_Li(self, cameraSample.screen, lightPath, rng, numRays);
  }

  screenSample.alpha = 1.f;
  screenSample.z = inf;
  screenSample.rgb = L * rcpf(spp);
  return screenSample;
}




void PathTracer_beginFrame(uniform Renderer *uniform _self,
                           uniform FrameBuffer *uniform fb)
{
//  print("pathtracer new frame %\n",fb->accumID);
  _self->fb = fb;
  uniform PathTracer  *uniform self = (uniform PathTracer *uniform)_self;
//  print("rays: %\n", self->numRays);
  self->numRays = 0;
}


task void PathTracer_renderTileJob(uniform PathTracer *uniform self,
                                   uniform Tile &tile,
                                   varying uint32 *uniform numRaysPerJob)
{
  uniform FrameBuffer *uniform fb   = self->super.fb;
  uniform int32 spp = self->super.spp;
  numRaysPerJob[taskIndex] = 0;

  const uniform int blocks = fb->accumID > 0 || spp > 0 ?
                               1 : min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);

  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = min(begin + RENDERTILE_PIXELS_PER_JOB, TILE_SIZE*TILE_SIZE/blocks);

  for (uint32 i=begin+programIndex;i<end;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i*blocks];
    const uint32 iy = tile.region.lower.y + z_order.ys[i*blocks];
    if (ix >= fb->size.x || iy >= fb->size.y)
      continue;

    uint32 numRays = 0;
    ScreenSample screenSample = PathTracer_renderPixel(self, ix, iy, numRays);
    numRaysPerJob[taskIndex] = numRaysPerJob[taskIndex] + numRays;

    for (uniform int p = 0; p < blocks; p++) {
      const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
      setRGBAZ(tile, pixel, screenSample.rgb, screenSample.alpha, screenSample.z);
    }
  }
}

void PathTracer_renderTile(uniform Renderer *uniform _self, uniform Tile &tile)
{
  uniform PathTracer *uniform self = (uniform PathTracer *uniform)_self;
  uniform int32 spp = self->super.spp;
  const uniform int blocks = self->super.fb->accumID > 0 || spp > 0 ? 1 : min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);
  uniform const size_t numJobs = ((TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB + blocks-1)/blocks;
  varying uint32 numRaysPerJob[(TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB];

  launch[numJobs] PathTracer_renderTileJob(self,tile,&numRaysPerJob[0]);
  sync;

  uint32 numRaysInTile = 0;
  for (uniform int i=0; i<numJobs; i++)
    numRaysInTile += numRaysPerJob[i];
  self->numRays += reduce_add(numRaysInTile);
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
  //self->minContribution = minContribution;
  self->minContribution = 0.0f;
  self->backplate = (uniform Image *uniform)backplate;
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
