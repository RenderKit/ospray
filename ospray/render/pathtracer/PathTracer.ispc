#include "PathTracer.ih"
#include "Scene.ih"

#define PDF_CULLING 0.0f 

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
    vec2f s  = PrecomputedSample__getVec2f(sample_,THIS->firstScatterSampleID     + lightPath.depth);
    float ss = PrecomputedSample__getFloat(sample_,THIS->firstScatterTypeSampleID + lightPath.depth);
    vec3f c = CompositedBRDF__sample(&brdfs,wo,dg,wi,type,s,ss,giBRDFTypes);
    
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

inline vec3f PathTracer_renderPixel(uniform PathTracer *uniform THIS,
                                    uniform Tile &tile, 
                                    uniform Random& rnd,
                                    const uint32 ix, 
                                    const uint32 iy,
                                    const float du,
                                    const float dv)
{
  uniform FrameBuffer *uniform fb = THIS->inherited.fb;

  uint32 numRays = 0;
  vec3f L = make_vec3f(0.f);
  uniform int set = Random__getInt(&rnd);
  uniform Camera *uniform camera = THIS->inherited.camera;
  CameraSample cameraSample;
  ScreenSample screenSample;

  screenSample.sampleID.x        = ix;
  screenSample.sampleID.y        = iy;
  
  for (uniform int s=0; s<THIS->inherited.spp; s++) {
    screenSample.sampleID.z        = THIS->iteration*THIS->inherited.spp+s;
    uniform PrecomputedSample* uniform sample = 
      PrecomputedSampler__get(&THIS->sampler,set,THIS->iteration*THIS->inherited.spp+s);

    const vec2f pixelSample = PrecomputedSample__getPixel(sample);
    const vec2f lensSample  = PrecomputedSample__getLens(sample);
    // const vec2f screenSample = (make_vec2f(ix,iy)+pixelSample) * tile.rcp_fbSize;

    cameraSample.screen.x = (screenSample.sampleID.x + pixelSample.x) * fb->rcpSize.x;
    cameraSample.screen.y = (screenSample.sampleID.y + pixelSample.y) * fb->rcpSize.y;
    cameraSample.lens     = lensSample;

    //    Ray ray;
    // THIS->camera->initRay(THIS->camera,ray,screenSample,lensSample);
    camera->initRay(camera,screenSample.ray,cameraSample);
    screenSample.ray.time = PrecomputedSample__getTime(sample); 

    LightPath lightPath; init_LightPath(lightPath,screenSample.ray);
    
    L = L+PathTraceIntegrator_Li(THIS,cameraSample.screen,lightPath,
                                 THIS->scene,sample,numRays);
  }
  return L * rcpf(THIS->inherited.spp);
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
 
void PathTracer_beginFrame(uniform Renderer *uniform renderer,
                           uniform FrameBuffer *uniform fb)
{
  // print("pathtracer new frame %\n",fb->accumID);
  uniform PathTracer* uniform THIS = (uniform PathTracer* uniform) renderer;
  if (PrecomputedSampler__empty(&THIS->sampler)) PathTracer__initSampler(THIS,THIS->scene);
  THIS->inherited.fb = fb;
}


void PathTracer_renderTile(uniform Renderer *uniform renderer,
                           uniform Tile &tile)
{
  
  uniform PathTracer  *uniform pt     = (uniform PathTracer *uniform)renderer;
  uniform FrameBuffer *uniform fb     = renderer->fb;
  uniform Camera      *uniform camera = renderer->camera;

  const uniform uint32 size_x = tile.fbSize.x;
  const uniform uint32 size_y = tile.fbSize.y;
  const uniform uint32 x0 = tile.region.lower.x;
  const uniform uint32 y0 = tile.region.lower.y;

  const uniform uint32 tile_x = x0 / TILE_SIZE;
  const uniform uint32 tile_y = y0 / TILE_SIZE;

  uniform Random rnd; 
  uniform int uniqueID
    = tile_x * 917*13*17 + tile_y * 81551*19*23
    + fb->accumID * 511 * 1023
    ; // iw: commented out + 3433*g_serverID;
  Random__setSeed(&rnd,uniqueID); // expensive

  float pixel_du = .5f, pixel_dv = .5f;
  float lens_du = 0.f, lens_dv = 0.f;
  if (fb->accumID >= 0) {
    // compute 
    pixel_du = precomputedHalton2(fb->accumID);
    pixel_dv = precomputedHalton3(fb->accumID);
  }
  
  ScreenSample screenSample;
  screenSample.sampleID.z = fb->accumID;
  screenSample.z = inf;
  screenSample.alpha = 0.f;
  
  for (uint32 i=programIndex;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
    const uint32 ix = tile.region.lower.x + z_order.xs[i];
    const uint32 iy = tile.region.lower.y + z_order.ys[i];
    if (ix >= fb->size.x || iy >= fb->size.y) 
      continue;
    screenSample.rgb = PathTracer_renderPixel(pt,tile,rnd,ix,iy,pixel_du,pixel_dv);
    const uint32 pixel = z_order.xs[i] + (z_order.ys[i] * TILE_SIZE);
     setRGBAZ(tile,pixel,screenSample.rgb,screenSample.alpha,screenSample.z);
  }
}

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
  uniform int maxSupportedDepth = min(MAX_SAMPLES_2D,MAX_SAMPLES_2D-1);
  if (THIS->maxDepth > maxSupportedDepth) {
    print("WARNING: setting recursion depth to maximum of %\n",maxSupportedDepth);
    THIS->maxDepth = maxSupportedDepth;
  }

  THIS->minContribution = minContribution;
  THIS->epsilon = epsilon;
  THIS->iteration = 0;
  THIS->backplate = backplate;

  THIS->lightSampleID = 0;
  THIS->firstScatterSampleID = 0;
  THIS->firstScatterTypeSampleID = 0;
  PrecomputedSampler__Constructor(&THIS->sampler,0,0);
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
