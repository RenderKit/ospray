// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathSampler.ih"
#include "PathStructs.ih"
#include "PathTracer.ih"
#include "PathTracerDefines.ih"
#include "PathTracerUtil.ih"

#include "bsdfs/BSDF.ih"
#include "bsdfs/MicrofacetAlbedoTables.ih"
#include "lights/Light.ih"
#include "materials/Material.ih"
#include "materials/Medium.ih"
#include "volumes/VolumeSampler.ih"

#include "camera/Camera.ih"
#include "fb/LocalFB.ih"
#include "math/Distribution1D.ih"
#include "math/random.ih"

ScreenSample PathTraceIntegrator_Li(const PathTracer *uniform self,
    const World *uniform world,
    FrameBuffer *uniform fb,
    const uint32 ix,
    const uint32 iy,
    const vec2f &pixel, // normalized, i.e. in [0..1]
    Ray &ray,
    varying LDSampler *uniform ldSampler,
    varying RandomSampler *uniform randomSampler)
{
  ScreenSample sample;
  sample.rgb = make_vec3f(0.f);
  sample.alpha = 1.f;
  sample.normal = make_vec3f(0.0f);
  sample.albedo = make_vec3f(0.0f);

  const uniform int numLights = world->pathtracerData.lights
      ? min(MAX_LIGHTS, world->pathtracerData.numLights)
      : 0;
  const uniform int numLightSamples =
      self->numLightSamples >= 0 ? self->numLightSamples : numLights;
  PathContext pathContext;
  pathContext.context = self;
  pathContext.world = world;
  pathContext.pixel = &pixel;
  pathContext.ldSampler = ldSampler;
  pathContext.randomSampler = randomSampler;
  pathContext.numLights = numLights;
  pathContext.numLightSamples = numLightSamples;
  pathContext.lightsCDF = world->pathtracerData.lightsCDF;

  pathContext.numBounceSampleDims =
      4 + numLightSamples * 3; // BSDF sample (3D), roulette (1D), light samples
                               // * (3D, light selection and sampling the light)

  PathState pathState;
  pathState.disableNEE = false;
  pathState.disableFWD = false;
  pathState.straightPath = true; // path from camera did not change direction,
                                 // for alpha and backplate
  pathState.specularTransmissionPath =
      true; // path from camera only has specular transmissions, for alpha and
            // backplate
  pathState.auxFree = true; // normal & albedo buffer were not yet written to
  pathState.depth = 0;
  pathState.sampleDim = 5; // skip: pixel (2D), lens (2D), time (1D)
  pathState.throughput = make_vec3f(1.f);
  pathState.contribution = make_vec3f(0.f);
  pathState.time = ray.time;
  pathState.currentMedium = make_Medium_vacuum();
  pathState.shadowCatcherDist = -inf;

  pathState.debug = false;

  samplePath(pathContext, pathState, ray, sample);

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
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  varying RandomSampler *uniform randomSampler = &randomSamplerObj;
  const int spp = max(1, self->super.spp);

  for (uniform int s = 0; s < spp; s++) {
    // init RNG
    const uint32 sampleID = accumID * spp + s;
    screenSample.sampleID.z = sampleID;
    LDSampler_init(ldSampler, fb->size.x * iy + ix, sampleID);
    RandomSampler_init(randomSampler, fb->size.x * iy + ix, sampleID);

    CameraSample cameraSample;
    const vec2f pixelSample = LDSampler_getFloat2(ldSampler, 0);

    vec2f pfSample = pixelSample;
    if (self->super.pixelFilter) {
      pfSample =
          self->super.pixelFilter->sample(self->super.pixelFilter, pixelSample)
          + make_vec2f(0.5f);
    }

    cameraSample.screen.x =
        (screenSample.sampleID.x + pfSample.x) * fb->rcpSize.x;
    cameraSample.screen.y =
        (screenSample.sampleID.y + pfSample.y) * fb->rcpSize.y;

    cameraSample.lens = LDSampler_getFloat2(ldSampler, 2);
    cameraSample.time = LDSampler_getFloat(ldSampler, 4);

    camera->initRay(camera, screenSample.ray, cameraSample);
    const float tMax = Renderer_getMaxDepth(&self->super, cameraSample.screen);
    screenSample.ray.t = min(screenSample.ray.t, tMax);

    ScreenSample sample = PathTraceIntegrator_Li(self,
        world,
        fb,
        ix,
        iy,
        cameraSample.screen,
        screenSample.ray,
        ldSampler,
        randomSampler);
    screenSample.rgb =
        screenSample.rgb + min(sample.rgb, make_vec3f(self->maxRadiance));
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
  const uniform int end =
      min(begin + RENDERTILE_PIXELS_PER_JOB, TILE_SIZE * TILE_SIZE);

  for (uint32 i = begin + programIndex; i < end; i += programCount) {
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
  PathTracer *uniform self = (PathTracer * uniform) _self;
  PathTracer_renderTileJob(self, fb, camera, world, tile, jobID);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void PathTracer_set(void *uniform _self,
    const uniform int32 rouletteDepth,
    const uniform float maxRadiance,
    const uniform vec4f &shadowCatcherPlane,
    const uniform int32 numLightSamples,
    const uniform bool backgroundRefraction)
{
  PathTracer *uniform self = (PathTracer * uniform) _self;

  self->rouletteDepth = rouletteDepth;
  self->maxRadiance = maxRadiance;
  self->numLightSamples = numLightSamples;
  self->backgroundRefraction = backgroundRefraction;

  uniform vec3f normal = make_vec3f(shadowCatcherPlane);
  const uniform float l = length(normal);
  self->shadowCatcher = l > 0.f;
  const uniform float rl = rcp(l);
  self->shadowCatcherPlane = make_vec4f(normal * rl, shadowCatcherPlane.w * rl);
}

export void *uniform PathTracer_create(void *uniform cppE)
{
  uniform PathTracer *uniform self = uniform new uniform PathTracer;
  Renderer_Constructor(&self->super, cppE);
  self->super.renderTile = PathTracer_renderTile;

  PathTracer_set(self, 5, inf, make_vec4f(0.f), 1, false);

  precomputeMicrofacetAlbedoTables();
  precomputeZOrder();

  return self;
}

