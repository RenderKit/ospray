// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathSampler.ih"
#include "PathStructs.ih"
#include "PathTracerDefines.ih"
#include "PathTracerUtil.ih"

#include "common/FeatureFlagsEnum.h"
#include "render/Renderer.ih"
#include "render/bsdfs/BSDF.ih"
#include "render/bsdfs/MicrofacetAlbedoTables.ih"
#include "render/materials/Medium.ih"
#include "render/util.ih"
#include "volumes/VolumeSampler.ih"

#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/World.ih"
#include "fb/FrameBuffer.ih"
#include "fb/FrameBufferDispatch.ih"
#include "fb/RenderTaskDesc.ih"
#include "math/Distribution1D.ih"
#include "math/random.ih"
#include "pf/PixelFilterDispatch.ih"
// c++ shared
#include "PathTracerDataShared.h"
#include "PathTracerShared.h"
#include "render/MaterialShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#define MAX_LIGHTS 1000u

static ScreenSample PathTraceIntegrator_Li(const PathTracer *uniform self,
    const World *uniform world,
    const vec2f &pixel, // normalized, i.e. in [0..1]
    Ray &ray,
    varying LDSampler *uniform ldSampler,
    varying RandomSampler *uniform randomSampler,
    const uniform FeatureFlagsHandler &ffh)
{
  ScreenSample sample;
  sample.rgb = make_vec3f(0.f);
  sample.alpha = 1.f;
  sample.normal = make_vec3f(0.0f);
  sample.albedo = make_vec3f(0.0f);
  sample.primID = RTC_INVALID_GEOMETRY_ID;
  sample.geomID = RTC_INVALID_GEOMETRY_ID;
  sample.instID = RTC_INVALID_GEOMETRY_ID;

  uniform PathTracerData &pathtracerData =
      *((uniform PathTracerData *)world->pathtracerData);
  const uniform uint32 numLights =
      pathtracerData.lights ? min(MAX_LIGHTS, pathtracerData.numLights) : 0;
  const uniform uint32 numLightSamples =
      self->numLightSamples >= 0 && numLights > 0 ? self->numLightSamples
                                                  : numLights;
  PathContext pathContext;
  pathContext.context = self;
  pathContext.world = world;
  pathContext.pixel = &pixel;
  pathContext.ldSampler = ldSampler;
  pathContext.randomSampler = randomSampler;
  pathContext.numLights = numLights;
  pathContext.numLightSamples = numLightSamples;
  pathContext.lightsCDF = pathtracerData.lightsCDF;

  // BSDF sample (3D), light samples * (3D, light selection and sampling)
  pathContext.numBounceSampleDims = 3 + numLightSamples * 3;

  PathState pathState;
  pathState.disableNEE = numLightSamples == 0;
  pathState.disableFWD = false;
  pathState.straightPath = true; // path from camera did not change direction,
                                 // for alpha and backplate
  pathState.specularTransmissionPath =
      true; // path from camera only has specular transmissions, for alpha and
            // backplate
  pathState.auxFree = true; // normal & albedo buffer were not yet written to
  pathState.depth = 0;
  pathState.sampleDim = 5; // skip: pixel (2D), lens (2D), time (1D)
  pathState.scatteringEvents = 0;
  pathState.throughput = make_vec3f(1.f);
  pathState.contribution = make_vec3f(0.f);
  pathState.time = ray.time;
  pathState.currentMedium = make_Medium_vacuum();
  pathState.shadowCatcherDist = -(float)inf;

  pathState.debug = false;

  samplePath(pathContext, pathState, ray, sample, ffh);

  return sample;
}

static ScreenSample PathTracer_renderPixel(PathTracer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 ix,
    const uint32 iy,
    const uint32 accumID,
    const uniform FeatureFlagsHandler &ffh)
{
  ScreenSample screenSample;
  screenSample.rgb = make_vec3f(0.f);
  screenSample.alpha = 0.f;
  screenSample.z = inf;
  screenSample.normal = make_vec3f(0.f);
  screenSample.albedo = make_vec3f(0.f);

  screenSample.sampleID.x = ix;
  screenSample.sampleID.y = iy;

  screenSample.primID = RTC_INVALID_GEOMETRY_ID;
  screenSample.geomID = RTC_INVALID_GEOMETRY_ID;
  screenSample.instID = RTC_INVALID_GEOMETRY_ID;

  LDSampler ldSamplerObj;
  RandomSampler randomSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  varying RandomSampler *uniform randomSampler = &randomSamplerObj;
  const int spp = max(1, self->super.spp);

  for (uniform int s = 0; s < spp; s++) {
    // init RNG
    const uint32 sampleID = accumID * spp + s;
    screenSample.sampleID.z = sampleID;
    LDSampler_init(
        ldSampler, self->super.mathConstants, fb->size.x * iy + ix, sampleID);
    RandomSampler_init(randomSampler, fb->size.x * iy + ix, sampleID);

    CameraSample cameraSample;
    const vec2f pixelSample = LDSampler_getFloat2(ldSampler, 0);

    vec2f pfSample = pixelSample;
    const PixelFilter *uniform pf = self->super.pixelFilter;
    if (pf) {
      pfSample =
          PixelFilter_dispatch_sample(pf, pixelSample) + make_vec2f(0.5f);
    }

    cameraSample.screen.x =
        (screenSample.sampleID.x + pfSample.x) * fb->rcpSize.x;
    cameraSample.screen.y =
        (screenSample.sampleID.y + pfSample.y) * fb->rcpSize.y;

    cameraSample.lens = LDSampler_getFloat2(ldSampler, 2);
    cameraSample.time = LDSampler_getFloat(ldSampler, 4);

    Camera_dispatch_initRay(camera, screenSample.ray, cameraSample, ffh);
    const float tMax =
        Renderer_getMaxDepth(&self->super, cameraSample.screen, ffh);

    screenSample.ray.t = min(screenSample.ray.t, tMax);

    ScreenSample sample = PathTraceIntegrator_Li(self,
        world,
        cameraSample.screen,
        screenSample.ray,
        ldSampler,
        randomSampler,
        ffh);

    screenSample.rgb =
        screenSample.rgb + min(sample.rgb, make_vec3f(self->maxRadiance));
    screenSample.alpha = screenSample.alpha + sample.alpha;
    screenSample.normal = screenSample.normal + sample.normal;
    screenSample.albedo = screenSample.albedo + sample.albedo;
    if (sample.z < screenSample.z) {
      screenSample.z = sample.z;
      screenSample.primID = sample.primID;
      screenSample.geomID = sample.geomID;
      screenSample.instID = sample.instID;
    }
  }

  const float rspp = rcpf(spp);
  screenSample.rgb = screenSample.rgb * rspp;
  screenSample.alpha = screenSample.alpha * rspp;
  screenSample.normal = screenSample.normal * rspp;
  screenSample.albedo = screenSample.albedo * rspp;

  return screenSample;
}

#ifndef OSPRAY_TARGET_SYCL
task
#endif
    SYCL_EXTERNAL void
    PathTracer_renderTask(Renderer *uniform _self,
        FrameBuffer *uniform fb,
        Camera *uniform camera,
        World *uniform world,
        const uint32 *uniform taskIDs,
#ifdef OSPRAY_TARGET_SYCL
        const int taskIndex0,
#endif
        const uniform FeatureFlagsHandler &ffh)
{
  PathTracer *uniform self = (PathTracer * uniform) _self;
  uniform RenderTaskDesc taskDesc =
      FrameBuffer_dispatch_getRenderTaskDesc(fb, taskIDs[taskIndex0], ffh);

  if (fb->cancelRender || isEmpty(taskDesc.region)) {
    return;
  }

#ifdef OSPRAY_TARGET_SYCL
  for (uint32 y = taskDesc.region.lower.y; y < (uint32)taskDesc.region.upper.y;
       ++y)
    for (uint32 x = taskDesc.region.lower.x;
         x < (uint32)taskDesc.region.upper.x;
         ++x) {
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
      ScreenSample screenSample = PathTracer_renderPixel(
          self, fb, camera, world, x, y, taskDesc.accumID, ffh);

      FrameBuffer_dispatch_accumulateSample(fb, screenSample, taskDesc, ffh);
    }
  FrameBuffer_dispatch_completeTask(fb, taskDesc, ffh);
}

// Exports (called from C++) //////////////////////////////////////////////////

#ifndef OSPRAY_TARGET_SYCL
export void PathTracer_renderTasks(void *uniform _self,
    void *uniform _fb,
    void *uniform _camera,
    void *uniform _world,
    void *uniform _taskIDs,
    uniform uint32 numTasks)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  FrameBuffer *uniform fb = (FrameBuffer * uniform) _fb;
  Camera *uniform camera = (Camera * uniform) _camera;
  World *uniform world = (World * uniform) _world;
  const uint32 *uniform taskIDs = (const uint32 *uniform)_taskIDs;
  uniform FeatureFlagsHandler ffh;
  launch[numTasks] PathTracer_renderTask(self, fb, camera, world, taskIDs, ffh);
}
#endif

OSPRAY_END_ISPC_NAMESPACE
