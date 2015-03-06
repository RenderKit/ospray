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
#include "Scene.ih"

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
  Ray    ray;                /*! Last ray in the path. */
  Medium lastMedium;             /*! Medium the last ray travels inside. */
  uniform uint32 depth;            /*! Recursion depth of path. */
  vec3f  throughput;             /*! Determines the fraction of
                                     radiance reaches the pixel along
                                     the path. */
  bool   ignoreVisibleLights;    /*! If the previous shade point used
                                     shadow rays we have to ignore the
                                     emission of geometrical lights to
                                     not double count them. */
  bool   unbent;                    /*! True of the ray path is a straight line. */
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

vec3f PathTraceIntegrator_Li(const uniform PathTracer* uniform THIS,
                             const vec2f &pixel, // normalized, i.e. in [0..1]
                             LightPath &lightPath, 
                             const uniform Scene *uniform scene,
                             varying RandomTEA* uniform rng,
                             uint32 &numRays)
{
  uniform uint32/*BRDFType*/ directLightingBRDFTypes = (uniform uint32)(DIFFUSE);
  uniform uint32/*BRDFType*/ giBRDFTypes = (uniform uint32)(ALL);
  
  vec3f L = make_vec3f(0.f);
  vec3f Lw = make_vec3f(1.f);

  while (true) 
  {
    /*! Terminate path if too long or contribution too low. */
    if (lightPath.depth >= THIS->maxDepth)
      return L;
    if (reduce_max(lightPath.throughput) <= THIS->minContribution)
      return L;
    
    /*! Traverse ray. */
    DifferentialGeometry dg;
    traceRay(scene->model,lightPath.ray);
    // rtcIntersect(scene->accel,lightPath.ray);
    postIntersect(scene,lightPath.ray,dg);
    numRays++;  
    
    const vec3f wo = neg(lightPath.ray.dir);

    /*! Environment shading when nothing hit. */
    if (noHit(lightPath.ray)) 
    {
      if ((bool)THIS->backplate & lightPath.unbent) {
        const int x = clamp((int)(pixel.x * THIS->backplate->size.x), 0, (int)THIS->backplate->size.x-1);
        const int y = clamp((int)(pixel.y * THIS->backplate->size.y), 0, (int)THIS->backplate->size.y-1);
        L = THIS->backplate->get_nearest_varying(THIS->backplate,x,y);
      }
      else {
        if (!lightPath.ignoreVisibleLights) {
          for (uniform int i=0; i<scene->num_envLights; i++) {
            uniform const EnvironmentLight *uniform l = scene->envLights[i]; 
            L = add(L, mul(Lw,l->Le(l,wo)));
          }      
        }
      }

      return L;
    }

    /*! Shade surface. */
    uniform CompositedBRDF brdfs;
    CompositedBRDF__Constructor(&brdfs);
#if 1
    uniform PathTraceMaterial*m = (uniform PathTraceMaterial*)dg.material;
    foreach_unique(mm in m)
    if (mm != NULL) mm->shade(mm,lightPath.ray, lightPath.lastMedium, dg, brdfs);
#else
    foreach_unique(geomID in lightPath.ray.geomID) {
      uniform PathTraceMaterial* uniform m
        = (uniform PathTraceMaterial*)(scene->model->geometry[geomID]->material);
      print("shading %\n",m);
      if (m != NULL) m->shade(m,lightPath.ray, lightPath.lastMedium, dg, brdfs);
    }
#endif

#if 0
    // iw: disabled because we dont' have per-geometry lights yet
    /*! Add light emitted by hit area light source. */
    if (!lightPath.ignoreVisibleLights) {
      foreach_unique(geomID in lightPath.ray.geomID) {
        const uniform AreaLight* uniform l = scene->model->geometry[geomID]->light;
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
      uniform int numAllLights = min(MAX_LIGHTS,scene->num_allLights);
      for (uniform int i=0; i<numAllLights; i++) 
      {
        uniform Light* uniform light = scene->allLights[i];

        /*! Either use precomputed samples for the light or sample light now. */
        LightSample ls; 
        ls.wi.v = make_vec3f(0.0f,0.0f,0.0f); ls.wi.pdf = 0.0f;
#if 0
        // iw: turned off precomputed, not yet working
        if (light->type & TY_PRECOMPUTE_LIGHT_SAMPLES) {
          ls = PrecomputedSample__getLightSample(sample_,THIS->precomputedLightSampleID[i]);
        }
        else 
#endif
          {
          ls.L = light->sample(light, dg, ls.wi, ls.tMax, RandomTEA__getFloats(rng));
        }

        /*! Ignore zero radiance or illumination from the back. */
        //if (reduce_max(ls.L) <= 0.0f | ls.wi.pdf <= PDF_CULLING | dot(dg.Ns,ls.wi.v) <= 1e-8f) 
        if (reduce_max(ls.L) <= 0.0f | ls.wi.pdf <= PDF_CULLING) 
          continue;

        /*! Evaluate BRDF */
        vec3f brdf = CompositedBRDF__eval(&brdfs,wo,dg,ls.wi.v,directLightingBRDFTypes);
#ifdef USE_DGCOLOR
        if (dg.color.x > -1.f)
          brdf = brdf * make_vec3f(dg.color);
#endif
        if (reduce_max(brdf) <= 0.0f)
          continue;
        /*! Test for shadows. */
        numRays++;
        Ray shadow_ray; 
        setRay(shadow_ray,dg.P,ls.wi.v,
               //dg.error*
               THIS->epsilon,ls.tMax-
               //dg.error*
               THIS->epsilon);
        shadow_ray.time = lightPath.ray.time;
        
        if (isOccluded(scene->model,shadow_ray))
          continue;

        L = add(L,mul(mul(Lw,ls.L),mul(brdf,rcp(ls.wi.pdf))));
      }
    }

    /*! Global illumination. Pick one BRDF component and sample it. */
    if (lightPath.depth >= THIS->maxDepth) 
      return L;
    
    /*! sample brdf */
    Sample3f wi = make_Sample3f(make_vec3f(0.0f),0.0f); uint32 type = 0;
    vec2f s  = RandomTEA__getFloats(rng);
    vec2f ss = RandomTEA__getFloats(rng);
    vec3f c = CompositedBRDF__sample(&brdfs, wo, dg, wi, type, s, ss.x, giBRDFTypes);
#ifdef USE_DGCOLOR
    if (dg.color.x > -1.f && (type & GLOSSY_REFLECTION) == NONE) // only colorize diffuse component
      c = c * make_vec3f(dg.color);
#endif
    
    /*! Continue only if we hit something valid. */
    if (reduce_max(c) <= 0.0f | wi.pdf <= PDF_CULLING) 
      return L;

    /*! Compute  simple volumetric effect. */
    const vec3f transmission = lightPath.lastMedium.transmission;
    if (ne(transmission,make_vec3f(1.f)))
      // c = mul(c, pow(transmission,lightPath.ray.t));
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
                THIS->epsilon,inf,
           c,(type & directLightingBRDFTypes) != NONE);

    Lw = mul(mul(Lw,c),rcp(wi.pdf));
  }
  return L;
}


export void PathTracer_setCamera(void *uniform _THIS, void *uniform _camera)
{
  uniform PathTracer *uniform THIS = (uniform PathTracer *uniform)_THIS;
  THIS->inherited.camera = (uniform Camera*)_camera;
}

export void PathTracer_setModel(void *uniform _THIS, void *uniform _model)
{
  uniform PathTracer *uniform THIS = (uniform PathTracer *uniform)_THIS;
  THIS->scene->model = (uniform Model*)_model;
  THIS->inherited.model = (uniform Model*)_model;
}

uniform EnvironmentLight* uniform AmbientLight__New(const uniform vec3f& L);
typedef uniform EnvironmentLight *uniform EnvLightPtr;
typedef uniform Light *uniform LightPtr;

uniform Scene *uniform createScene()
{
  uniform Scene *uniform scene = uniform new uniform Scene;
  scene->allLights = NULL;
  scene->num_allLights = 0;
  scene->envLights = NULL;
  scene->num_envLights = 0;

#if 1
  // we don't have lights, yet, so use a hardcoded envlight for now!
  print("#osp:PT: adding hard-coded envlight for now!\n");
  scene->envLights = uniform new uniform EnvLightPtr[1];
  scene->envLights[0] = AmbientLight__New(make_vec3f(.8f));
  scene->num_envLights = 1;

  scene->allLights = uniform new uniform LightPtr[1];
  scene->allLights[0] = (uniform LightPtr)scene->envLights[0];
  scene->num_allLights = 1;
#endif

  return scene;
}

inline ScreenSample PathTracer_renderPixel(uniform PathTracer *uniform THIS,
                                    const uint32 ix, 
                                    const uint32 iy)
{
  uniform FrameBuffer *uniform fb = THIS->inherited.fb;

  uint32 numRays = 0;
  vec3f L = make_vec3f(0.f);
  uniform Camera *uniform camera = THIS->inherited.camera;
  ScreenSample screenSample;

  screenSample.sampleID.x = ix;
  screenSample.sampleID.y = iy;

  // init RNG
  RandomTEA rng_state; varying RandomTEA* const uniform rng = &rng_state;
  RandomTEA__Constructor(rng, fb->size.x*iy+ix, fb->accumID);
  const int spp = max(1, THIS->inherited.spp);
  
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
    
    L = L + PathTraceIntegrator_Li(THIS, cameraSample.screen, lightPath,
                                   THIS->scene, rng, numRays);
  }

  screenSample.alpha = 1.f;
  screenSample.z = inf;
  screenSample.rgb = L * rcpf(spp);
  return screenSample;
}



 
void PathTracer_beginFrame(uniform Renderer *uniform renderer,
                           uniform FrameBuffer *uniform fb)
{
//  print("pathtracer new frame %\n",fb->accumID);
  renderer->fb = fb;
}

#if 1
task void PathTracer_renderTileJob(uniform Renderer *uniform renderer, uniform Tile &tile)
{
  const uniform int begin = taskIndex * RENDERTILE_PIXELS_PER_JOB;
  const uniform int end   = begin     + RENDERTILE_PIXELS_PER_JOB;

  uniform PathTracer  *uniform pt     = (uniform PathTracer *uniform)renderer;
  uniform FrameBuffer *uniform fb     = renderer->fb;

  const uniform int blocks = renderer->spp > 0 || fb->accumID > 0 ? 1 : min(1 << -2 * renderer->spp, TILE_SIZE*TILE_SIZE);
  
  for (uint32 i=begin+programIndex;i<end;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i*blocks];
    const uint32 iy = tile.region.lower.y + z_order.ys[i*blocks];
    if (ix >= fb->size.x || iy >= fb->size.y) 
      continue;

    ScreenSample screenSample = PathTracer_renderPixel(pt, ix, iy);
    for (uniform int p = 0; p < blocks; p++) {
      const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
      setRGBAZ(tile, pixel, screenSample.rgb, screenSample.alpha, screenSample.z);
    }
  }
}

void PathTracer_renderTile(uniform Renderer *uniform renderer, uniform Tile &tile)
{
  uniform const size_t numJobs = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
  launch[numJobs] PathTracer_renderTileJob(renderer,tile);
}
#else
void PathTracer_renderTile(uniform Renderer *uniform renderer, uniform Tile &tile)
{
  uniform PathTracer  *uniform pt     = (uniform PathTracer *uniform)renderer;
  uniform FrameBuffer *uniform fb     = renderer->fb;

  const uniform int blocks = renderer->spp > 0 || fb->accumID > 0 ? 1 : min(1 << -2 * renderer->spp, TILE_SIZE*TILE_SIZE);
  
  for (uint32 i=programIndex;i<TILE_SIZE*TILE_SIZE/blocks;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i*blocks];
    const uint32 iy = tile.region.lower.y + z_order.ys[i*blocks];
    if (ix >= fb->size.x || iy >= fb->size.y) 
      continue;

    ScreenSample screenSample = PathTracer_renderPixel(pt, ix, iy);
    for (uniform int p = 0; p < blocks; p++) {
      const uint32 pixel = z_order.xs[i*blocks+p] + (z_order.ys[i*blocks+p] * TILE_SIZE);
      setRGBAZ(tile, pixel, screenSample.rgb, screenSample.alpha, screenSample.z);
    }
  }
}
#endif

void PathTracer__Constructor(void *uniform cppE,
                             uniform PathTracer* uniform THIS,
                             const uniform int& maxDepth, 
                             const uniform float& minContribution,
                             const uniform float& epsilon,
                             uniform Image* uniform backplate)
{
  Renderer_constructor(&THIS->inherited,cppE,NULL,NULL);
  THIS->inherited.renderTile   = PathTracer_renderTile;
  THIS->inherited.beginFrame   = PathTracer_beginFrame;

  THIS->maxDepth = maxDepth;

  THIS->minContribution = minContribution;
  THIS->epsilon = epsilon;
  THIS->backplate = backplate;
}


export void* uniform PathTracer_create(void *uniform cppE,
                                       const uniform int32& maxDepth, 
                                       const uniform float& minContribution,
                                       const uniform float& epsilon,
                                       void* uniform backplate)
{
  uniform PathTracer *uniform THIS = uniform new uniform PathTracer;
  PathTracer__Constructor(cppE,THIS,maxDepth,minContribution,epsilon,
                          (uniform Image* uniform) backplate);

  precomputeZOrder();
  
  THIS->scene = createScene();
  return THIS;
}
