#include "pathtracer.ih"
#include "scene.ih"


#define PDF_CULLING 0.0f 
#if 0

#include "renderer.isph"
#include "materials/medium.isph"
#include "materials/material.isph"
#include "lights/light.isph"
#include "brdfs/compositedbrdf.isph"
#include "textures/image.isph"
#include "framebuffers/framebuffer.isph"

#if defined (ISPC_TARGET_SSE2) || defined (ISPC_TARGET_SSE4)
#  define PACKET_WIDTH 2
#  define PACKET_HEIGHT 2
#elif defined (ISPC_TARGET_AVX) || defined(ISPC_TARGET_AVX2)
#  define PACKET_WIDTH 4
#  define PACKET_HEIGHT 2
#else
#  define PACKET_WIDTH 4
#  define PACKET_HEIGHT 4
#endif

#define TILE_SIZE_X 8
#define TILE_SIZE_Y 8

#endif

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
                             const vec2f &pixel,
                             LightPath &lightPath, 
                             const uniform Scene *uniform scene,
                             const uniform PrecomputedSample* uniform sample_,
                             uint32 &numRays)
{
  //uniform uint/*BRDFType*/ directLightingBRDFTypes = (uniform uint32)(DIFFUSE|GLOSSY);
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
// #if 0
//     uniform Material* material = scene->model->geometry[lightPath.ray.geomID]->material;
//     foreach_unique(m in material) 
//       if (m != NULL) m->shade(m,lightPath.ray, lightPath.lastMedium, dg, brdfs);
// #else
    // print("HIT.geomID: %\n",lightPath.ray.geomID);
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
// #endif

    // L = abs(dg.Ns);
    // L = make_vec3f(abs(dot(lightPath.ray.dir,dg.Ns)));

#if 0
    // iw: disabled because we dont' have per-geometry lights yet
    /*! Add light emitted by hit area light source. */
    if (!lightPath.ignoreVisibleLights) {
      foreach_unique(geomID in lightPath.ray.geomID) {
        const uniform AreaLight* uniform l = scene->model->geometry[geomID]->light;
        if (l != NULL) L = add(L, mul(Lw, l->Le(l,dg,wo)));
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
          ls.L = light->sample(light, dg, ls.wi, ls.tMax, 
                               PrecomputedSample__getVec2f(sample_,THIS->lightSampleID));
        }

        /*! Ignore zero radiance or illumination from the back. */
        //if (reduce_max(ls.L) <= 0.0f | ls.wi.pdf <= PDF_CULLING | dot(dg.Ns,ls.wi.v) <= 1e-8f) 
        if (reduce_max(ls.L) <= 0.0f | ls.wi.pdf <= PDF_CULLING) 
          continue;

        /*! Evaluate BRDF */
        vec3f brdf = CompositedBRDF__eval(&brdfs,wo,dg,ls.wi.v,directLightingBRDFTypes);
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
        
        // rtcOccluded(scene->accel,shadow_ray);
        // if (hadHit(shadow_ray)) continue; 
        if (isOccluded(scene->model,shadow_ray))
          continue;

        L = add(L,mul(mul(Lw,ls.L),mul(brdf,rcp(ls.wi.pdf))));
      }
    }

    // iw: force exit here
    return L;

#if 0

    /*! Global illumination. Pick one BRDF component and sample it. */
    if (lightPath.depth >= THIS->maxDepth) 
      return L;
    
    /*! sample brdf */
    Sample3f wi = make_Sample3f(make_vec3f(0.0f),0.0f); uint32 type = 0;
    vec2f s  = PrecomputedSample__getVec2f(sample_,THIS->firstScatterSampleID     + lightPath.depth);
    float ss = PrecomputedSample__getFloat(sample_,THIS->firstScatterTypeSampleID + lightPath.depth);
    vec3f c = CompositedBRDF__sample(&brdfs,wo,dg,wi,type,s,ss,giBRDFTypes);
    
    /*! Continue only if we hit something valid. */
    if (reduce_max(c) <= 0.0f | wi.pdf <= PDF_CULLING) 
      return L;

    /*! Compute  simple volumetric effect. */
    const vec3f transmission = lightPath.lastMedium.transmission;
    if (ne(transmission,make_vec3f(1.f)))
      c = mul(c, pow(transmission,lightPath.ray.tfar));
    
    /*! Tracking medium if we hit a medium interface. */
    if (type & TRANSMISSION) {
#if 0
      foreach_unique(m in material) 
        if (m != NULL)  m->selectNextMedium(m,lightPath.lastMedium);
#else
      foreach_unique(geomID in lightPath.ray.id0) {
        uniform Material* uniform m = scene->geometry[geomID]->material;
        if (m != NULL)  m->selectNextMedium(m,lightPath.lastMedium);
      }
#endif
    }
    
    /*! Continue the path. */
    extend_fast(lightPath, 
           dg.P,wi.v,dg.error*THIS->epsilon,inf,
           c,(type & directLightingBRDFTypes) != NONE);

    Lw = mul(mul(Lw,c),rcp(wi.pdf));
#endif
  }
  return L;
}

#if 0
inline vec3f PathTracer__renderPixel(const uniform PathTracer* uniform THIS,
                                     const uniform Camera *uniform camera,
                                     const uniform Scene  *uniform scene,
                                     const uniform FrameBuffer *uniform fb,
                                     uniform Random& rnd,
                                     const uint32 ix, const uint32 iy, 
                                     uint32 &numRays)
{
  vec3f L = make_vec3f(0.f);
  uniform int set = Random__getInt(&rnd);
  for (uniform int s=0; s<THIS->spp; s++) 
    {
      uniform PrecomputedSample* uniform sample = 
        PrecomputedSampler__get(&THIS->sampler,set,THIS->iteration*THIS->spp+s);
    
      const vec2f pixelSample = PrecomputedSample__getPixel(sample);
      const vec2f lensSample  = PrecomputedSample__getLens(sample);
      const vec2f screenSample = mul(add(make_vec2f(ix,iy),pixelSample),fb->invSize);
    
      Ray ray;
      camera->initRay(camera,ray,screenSample,lensSample);
      ray.time = lensSample.x; // FIXME: introduced correlation
      LightPath lightPath; init_LightPath(lightPath,ray);
      L = add(L, PathTraceIntegrator_Li(THIS,screenSample,lightPath,scene,sample,numRays));
    }
  return mul(L, rcp(THIS->spp));
} 

task void PathTracer__renderTile(uniform PathTracer* uniform THIS,
                                 const uniform Camera *uniform camera,
                                 const uniform Scene  *uniform scene,
                                 const uniform ToneMapper* uniform toneMapper,
                                 uniform FrameBuffer *uniform fb,
                                 uniform AccuBuffer *uniform accu,
                                 const uniform int accuMode,
                                 const uniform uint32 numTiles_x) 
{
  uint32 numRays = 0;
  const uniform uint32 tile_y = taskIndex / numTiles_x;
  const uniform uint32 tile_x = taskIndex - tile_y * numTiles_x;
  const uint32 sample_y = programIndex / PACKET_WIDTH; 
  const uint32 sample_x = programIndex - sample_y * PACKET_WIDTH;

  uniform Random rnd; 
  uniform int uniqueID = tile_x * 917 + tile_y * 81551 + 3433*g_serverID;
  Random__setSeed(&rnd,uniqueID); // expensive
  
  const uniform uint32 tile_y0 = tile_y * TILE_SIZE_Y;
  const uniform uint32 tile_x0 = tile_x * TILE_SIZE_X;

  for (uniform uint32 iy=0; iy<TILE_SIZE_Y; iy+=PACKET_HEIGHT)
  {
    const uint32 y = (tile_y0 + iy) + sample_y;
    if (y >= fb->size.y) continue;
    
    if (!activeLine(y)) continue;
    size_t _y = raster2buffer(y);

    for (uniform unsigned int ix=0; ix<TILE_SIZE_X; ix+=PACKET_WIDTH) 
    { 
      const uint32 x = (tile_x0 + ix) + sample_x;
      if (x >= fb->size.x) continue;

      vec3f R = PathTracer__renderPixel(THIS,camera,scene,fb,rnd,x,y,numRays);
      vec3f d = AccuBuffer__update(accu,x,_y,R,accuMode);
      if (toneMapper) d = toneMapper->toneMap(toneMapper,d,x,y,fb->size);
      fb->set(fb,x,_y,d);
    }
  }

  /* count number of rays */
  uniform int num = 0;
  foreach_active(i) {
    num += extract(numRays,i);
  }
  atomic_add_global(&THIS->numRays,num);
}

void PathTracer__initSampler(uniform PathTracer* uniform THIS, const uniform Scene* uniform scene)
{
  PrecomputedSampler__reset(&THIS->sampler);
  PrecomputedSampler__Constructor(&THIS->sampler,16*16,64); // first argument HAS to be a square number, both have to be a power of two

  THIS->lightSampleID = PrecomputedSampler__request2D(&THIS->sampler,1);
  uniform int numAllLights = min(MAX_LIGHTS,scene->num_allLights);
  for (uniform int i=0; i<numAllLights; i++) {
    THIS->precomputedLightSampleID[i] = -1;
    if (scene->allLights[i]->type & TY_PRECOMPUTE_LIGHT_SAMPLES) 
      THIS->precomputedLightSampleID[i] = PrecomputedSampler__requestLightSample(&THIS->sampler,THIS->lightSampleID,scene->allLights[i]);
  }
  THIS->firstScatterSampleID = PrecomputedSampler__request2D(&THIS->sampler,THIS->maxDepth);
  THIS->firstScatterTypeSampleID = PrecomputedSampler__request1D(&THIS->sampler,THIS->maxDepth);
  PrecomputedSampler__init(&THIS->sampler);
}
 
void PathTracer_renderFrameInit(uniform Renderer* uniform _THIS,
                                const uniform Scene* uniform scene)
{
  uniform PathTracer* uniform THIS = (uniform PathTracer* uniform) _THIS;
  if (PrecomputedSampler__empty(&THIS->sampler)) PathTracer__initSampler(THIS,scene);
}

uniform int PathTracer_renderFrame(uniform Renderer* uniform _THIS,
                                   const uniform Camera* uniform camera,
                                   const uniform Scene* uniform scene,
                                   const uniform ToneMapper* uniform toneMapper,
                                   uniform SwapChain *uniform swapchain,
                                   const uniform int accuMode)
{
  uniform PathTracer* uniform THIS = (uniform PathTracer* uniform) _THIS;
  THIS->numRays = 0;
  if (accuMode == 0) THIS->iteration = 0;
  uniform int numTiles_x = (swapchain->width+ (TILE_SIZE_X-1))/TILE_SIZE_X;
  uniform int numTiles_y = (swapchain->height+(TILE_SIZE_Y-1))/TILE_SIZE_Y;
  uniform int numTiles = numTiles_x * numTiles_y;
  uniform FrameBuffer* uniform fb = SwapChain__get_buffer(swapchain);
  uniform AccuBuffer* uniform accu = SwapChain__get_accu(swapchain);
  launch[numTiles] PathTracer__renderTile(THIS,camera,scene,toneMapper,fb,accu,accuMode,numTiles_x);
  sync;

  rtcDebug();
  THIS->iteration++;
  return THIS->numRays;
}

void PathTracer__Destructor(uniform RefCount* uniform _THIS)
{
  uniform PathTracer* uniform THIS = (uniform PathTracer* uniform) _THIS;
  PrecomputedSampler__Destructor(&THIS->sampler);
  RefCount__DecRef(&THIS->backplate->base);
  Renderer__Destructor(_THIS);
}

#endif


void PathTracer__Constructor(uniform PathTracer* uniform THIS,
                             const uniform int& maxDepth, 
                             const uniform float& minContribution,
                             const uniform float& epsilon,
                             const uniform int& spp,
                             uniform Image* uniform backplate)
{
  // Renderer__Constructor(&THIS->base,PathTracer__Destructor,PathTracer_renderFrameInit,PathTracer_renderFrame);

  THIS->maxDepth = maxDepth;
  uniform int maxSupportedDepth = min(MAX_SAMPLES_2D,MAX_SAMPLES_2D-1);
  if (THIS->maxDepth > maxSupportedDepth) {
    print("WARNING: setting recursion depth to maximum of %\n",maxSupportedDepth);
    THIS->maxDepth = maxSupportedDepth;
  }

  THIS->minContribution = minContribution;
  THIS->epsilon = epsilon;
  THIS->spp = spp;
  THIS->iteration = 0;
  // RefCount__IncRef(&backplate->base);
  THIS->backplate = backplate;

  THIS->lightSampleID = 0;
  THIS->firstScatterSampleID = 0;
  THIS->firstScatterTypeSampleID = 0;
  PrecomputedSampler__Constructor(&THIS->sampler,0,0);
}

export void PathTracer_setCamera(void *uniform _THIS, void *uniform _camera)
{
  uniform PathTracer *uniform THIS = (uniform PathTracer *uniform)_THIS;
  THIS->camera = (uniform Camera*)_camera;
}

export void PathTracer_setModel(void *uniform _THIS, void *uniform _model)
{
  uniform PathTracer *uniform THIS = (uniform PathTracer *uniform)_THIS;
  THIS->scene->model = (uniform Model*)_model;
}

uniform EnvironmentLight* uniform AmbientLight__New(const uniform vec3f& L);
typedef uniform EnvironmentLight *uniform EnvLightPtr;
typedef uniform Light *uniform LightPtr;

uniform Scene *uniform createScene()
{
  uniform Scene *uniform scene = uniform new uniform Scene;
  scene->allLights = NULL;
  scene->num_allLights = NULL;
  scene->envLights = NULL;
  scene->num_envLights = NULL;

  #if 1
  // we don't have lights, yet, so use a hardcoded envlight for now!
  print("#osp:pt: adding hard-coded envlight for now!\n");
  scene->envLights = uniform new uniform EnvLightPtr[1];
  scene->envLights[0] = AmbientLight__New(make_vec3f(1.f));
  scene->num_envLights = 1;

  scene->allLights = uniform new uniform LightPtr[1];
  scene->allLights[0] = (uniform LightPtr)scene->envLights[0];
  scene->num_allLights = 1;
#endif

  return scene;
}

export void* uniform PathTracer_create(const uniform int32& maxDepth, 
                                       const uniform float& minContribution,
                                       const uniform float& epsilon,
                                       const uniform int32& spp,
                                       void* uniform backplate)
{
  uniform PathTracer *uniform THIS = uniform new uniform PathTracer;
  PathTracer__Constructor(THIS,maxDepth,minContribution,epsilon,spp,
                          (uniform Image* uniform) backplate);
  
  THIS->scene = createScene();
  return THIS;
}





// inline vec3f PathTracer__renderPixel(const uniform PathTracer* uniform THIS,
//                                      const uniform Camera *uniform camera,
//                                      const uniform Model  *uniform scene,
//                                      // const uniform Scene  *uniform scene,
//                                      const uniform FrameBuffer *uniform fb,
//                                      uniform Random& rnd,
//                                      const uint32 ix, const uint32 iy, 
//                                      uint32 &numRays)
// {
// #if 0
//   vec3f L = make_vec3f(0.f);
//   uniform int set = Random__getInt(&rnd);
//   for (uniform int s=0; s<THIS->spp; s++) 
//   {
//     uniform PrecomputedSample* uniform sample = 
//       PrecomputedSampler__get(&THIS->sampler,set,THIS->iteration*THIS->spp+s);
    
//     const vec2f pixelSample = PrecomputedSample__getPixel(sample);
//     const vec2f lensSample  = PrecomputedSample__getLens(sample);
//     const vec2f screenSample = mul(add(make_vec2f(ix,iy),pixelSample),fb->invSize);
    
//     Ray ray;
//     camera->initRay(camera,ray,screenSample,lensSample);
//     ray.time = lensSample.x; // FIXME: introduced correlation
//     LightPath lightPath; init_LightPath(lightPath,ray);
//     L = add(L, PathTraceIntegrator_Li(THIS,screenSample,lightPath,scene,sample,numRays));
//   }
//   return mul(L, rcp(THIS->spp));
// #endif
// } 
inline vec3f PathTracer_renderPixel(uniform PathTracer *uniform THIS,
                                    uniform Tile &tile, 
                                    uniform Random& rnd,
                                    const uint32 ix, 
                                    const uint32 iy)
{
  uint32 numRays = 0;
  vec3f L = make_vec3f(0.f);
  uniform int set = Random__getInt(&rnd);
  for (uniform int s=0; s<THIS->spp; s++) {
    uniform PrecomputedSample* uniform sample = 
      PrecomputedSampler__get(&THIS->sampler,set,THIS->iteration*THIS->spp+s);
    const vec2f pixelSample = PrecomputedSample__getPixel(sample);
    const vec2f lensSample  = PrecomputedSample__getLens(sample);
    const vec2f screenSample = (make_vec2f(ix,iy)+pixelSample) * tile.rcp_fbSize;
    
    Ray ray;
    THIS->camera->initRay(THIS->camera,ray,screenSample,lensSample);
    ray.time = lensSample.x; // FIXME: introduced correlation

    LightPath lightPath; init_LightPath(lightPath,ray);
    
    // L = L + abs(ray.dir);
    // L = L + make_vec3f(lensSample.x, lensSample.y, 0.f);
    L = L+PathTraceIntegrator_Li(THIS,screenSample,lightPath,THIS->scene,sample,numRays);
  }
  return L * rcpf(THIS->spp);
}




void PathTracer__initSampler(uniform PathTracer* uniform THIS, const uniform Scene* uniform scene)
{
  PrecomputedSampler__reset(&THIS->sampler);
  PrecomputedSampler__Constructor(&THIS->sampler,16*16,64); // first argument HAS to be a square number, both have to be a power of two

  THIS->lightSampleID = PrecomputedSampler__request2D(&THIS->sampler,1);
  uniform int numAllLights = min(MAX_LIGHTS,scene->num_allLights);
  for (uniform int i=0; i<numAllLights; i++) {
    THIS->precomputedLightSampleID[i] = -1;
    if (scene->allLights[i]->type & TY_PRECOMPUTE_LIGHT_SAMPLES) 
      THIS->precomputedLightSampleID[i] = PrecomputedSampler__requestLightSample(&THIS->sampler,THIS->lightSampleID,scene->allLights[i]);
  }
  THIS->firstScatterSampleID = PrecomputedSampler__request2D(&THIS->sampler,THIS->maxDepth);
  THIS->firstScatterTypeSampleID = PrecomputedSampler__request1D(&THIS->sampler,THIS->maxDepth);
  PrecomputedSampler__init(&THIS->sampler);
}
 
export void PathTracer_renderFrameInit(void * uniform _THIS)
{
  uniform PathTracer* uniform THIS = (uniform PathTracer* uniform) _THIS;
  if (PrecomputedSampler__empty(&THIS->sampler)) PathTracer__initSampler(THIS,THIS->scene);
}



export void PathTracer_renderTile(uniform Tile &tile, 
                                  void *uniform _pathtracer)
{
  // uint32 numRays = 0;
  // const uniform uint32 tile_y = taskIndex / numTiles_x;
  // const uniform uint32 tile_x = taskIndex - tile_y * numTiles_x;
  // const uint32 sample_y = programIndex / PACKET_WIDTH; 
  // const uint32 sample_x = programIndex - sample_y * PACKET_WIDTH;

  // uniform Random rnd; 
  // uniform int uniqueID = tile_x * 917 + tile_y * 81551 + 3433*g_serverID;
  // Random__setSeed(&rnd,uniqueID); // expensive
  
  // const uniform uint32 tile_y0 = tile_y * TILE_SIZE_Y;
  // const uniform uint32 tile_x0 = tile_x * TILE_SIZE_X;

  // for (uniform uint32 iy=0; iy<TILE_SIZE_Y; iy+=PACKET_HEIGHT)
  // {
  //   const uint32 y = (tile_y0 + iy) + sample_y;
  //   if (y >= fb->size.y) continue;
    
  //   if (!activeLine(y)) continue;
  //   size_t _y = raster2buffer(y);

  //   for (uniform unsigned int ix=0; ix<TILE_SIZE_X; ix+=PACKET_WIDTH) 
  //   { 
  //     const uint32 x = (tile_x0 + ix) + sample_x;
  //     if (x >= fb->size.x) continue;

  //     vec3f R = PathTracer__renderPixel(THIS,camera,scene,fb,rnd,x,y,numRays);
  //     vec3f d = AccuBuffer__update(accu,x,_y,R,accuMode);
  //     if (toneMapper) d = toneMapper->toneMap(toneMapper,d,x,y,fb->size);
  //     fb->set(fb,x,_y,d);
  //   }
  // }

  // /* count number of rays */
  // uniform int num = 0;
  // foreach_active(i) {
  //   num += extract(numRays,i);
  // }
  // atomic_add_global(&THIS->numRays,num);


  // uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform PathTracer *uniform THIS = (uniform PathTracer *uniform)_pathtracer;
  tile.format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile.fbSize.x;
  const uniform uint32 size_y = tile.fbSize.y;
  const uniform uint32 x0 = tile.region.lower.x;
  const uniform uint32 y0 = tile.region.lower.y;

  const uniform uint32 tile_x = x0 / TILE_SIZE;
  const uniform uint32 tile_y = y0 / TILE_SIZE;

  uniform Random rnd; 
  uniform int uniqueID = tile_x * 917 + tile_y * 81551; // iw: commented out + 3433*g_serverID;
  Random__setSeed(&rnd,uniqueID); // expensive

  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    // print("pixel % %\n",x,y);
    if (x < size_x & y < size_y) {
      //      vec3f R = PathTracer__renderPixel(THIS,camera,scene,fb,rnd,x,y,numRays);
      vec3f R = PathTracer_renderPixel(THIS,tile,rnd,x,y);

      // const vec2f pixelSample = PrecomputedSample__getPixel(sample);
      // const vec2f lensSample  = PrecomputedSample__getLens(sample);
      // const vec2f screenSample = mul(add(make_vec2f(ix,iy),pixelSample),fb->invSize);

      // // print("+pixel % %\n",x,y);
      // const float screen_u = (x+.5f)/size_x;
      // const float screen_v = (y+.5f)/size_y;
      // Ray ray;
      // THIS->camera->initRay(THIS->camera,ray,
      //                       make_vec2f(screen_u,screen_v),
      //                       (varying vec2f)make_vec2f(0.f,0.f)); //make_vec2f(0.f,0.f));
//       // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
//       // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);
// #if 0
//       rtcIntersect(scene,(varying RTCRay&)ray);
// #else
//       rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
// #endif
      // traceRay(THIS->model,ray);
      // const vec4f col = make_vec4f(R); //make_vec4f(screen_u,screen_v,screen_u+screen_v,0.f);
      // if (ray.geomID >= 0)
      //   print("col % % %\n",col.x,col.y,col.z);
      tile.r[i] = R.x;
      tile.g[i] = R.y;
      tile.b[i] = R.z;
      tile.a[i] = 0.f;
      tile.z[i] = 0.f; 
      tile.rgba8[i] = cvt_uint32(R);
    }
  }
}
