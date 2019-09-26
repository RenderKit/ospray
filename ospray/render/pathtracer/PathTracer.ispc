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

#include "render/pathtracer/PathTracer.ih"
#include "render/pathtracer/PathSampler.ih"
#include "render/pathtracer/PathTracerUtil.ih"
#include "render/pathtracer/PathTracerDefines.ih"
#include "render/pathtracer/PathStructs.ih"

#include "render/pathtracer/bsdfs/BSDF.ih"
#include "render/pathtracer/bsdfs/MicrofacetAlbedoTables.ih"
#include "render/pathtracer/materials/Medium.ih"
#include "render/pathtracer/materials/Material.ih"
#include "render/pathtracer/volumes/VolumeSampler.ih"

#include "camera/Camera.ih"
#include "math/random.ih"
#include "fb/LocalFB.ih"
#include "volume/VolumetricModel.ih"

inline void tracePathSimple(const PathContext& pathContext, 
                            PathState& pathState, 
                            Ray& ray, 
                            ScreenSample& sample)
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
  pathState.shadowCatcherDist = -inf;

  pathState.debug = false;
  //vec2f debug_pixel = make_vec2f(0.45f, 0.45f);
  //if (ix == (int)(debug_pixel.x*fb->size.x) && iy == (int)(debug_pixel.y*fb->size.y))
  //{
  //  pathState.debug = true;
  //}

  samplePath(pathContext, pathState, ray, sample);

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
}

export void PathTracer_setLights(void *uniform _self,
    void **uniform lights,
    const uniform uint32 numLights,
    const uniform uint32 numGeoLights)
{
  PathTracer *uniform self = (PathTracer * uniform) _self;

  self->lights = (const uniform Light *uniform *uniform)lights;
  self->numLights = numLights;
  self->numGeoLights = numGeoLights;
}

export void* uniform PathTracer_create(void *uniform cppE)
{
  uniform PathTracer *uniform self = uniform new uniform PathTracer;
  Renderer_Constructor(&self->super, cppE);
  self->super.renderTile = PathTracer_renderTile;

  PathTracer_set(self, 5, inf, NULL, make_vec4f(0.f));
  PathTracer_setLights(self, NULL, 0, 0);

  precomputeMicrofacetAlbedoTables();
  precomputeZOrder();

  return self;
}
