// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathSampler.ih"
#include "PathStructs.ih"
#include "PathTracerDefines.ih"
#include "PathTracerUtil.ih"

#include "common/DeviceRT.ih"
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
#include "common/RayCone.ih"
#include "render/MaterialShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#define MAX_LIGHT_SAMPLES 1024u

static ScreenSample PathTraceIntegrator_Li(PathContext &pathContext,
    PathState &pathState,
    Ray &ray,
    RayCone &rayCone,
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

  pathState.straightPath = true; // path from camera did not change direction,
                                 // for alpha and backplate
  pathState.specularTransmissionPath =
      true; // path from camera only has specular transmissions, for alpha and
            // backplate
  pathState.depth = 0;
  pathState.scatteringEvents = 0;
  pathState.firstBounceLight = true;
  pathState.throughput = make_vec3f(1.f);
  pathState.contribution = make_vec3f(0.f);
  pathState.time = ray.time;
  pathState.currentMedium = make_Medium_vacuum();
  pathState.shadowCatcherDist = -(float)inf;

  samplePath(pathContext, pathState, ray, rayCone, sample, ffh);

  return sample;
}

static inline uniform uint32 clz(uniform uint32 x)
{
#ifdef OSPRAY_TARGET_SYCL
  return sycl::clz(x);
#else
  return count_leading_zeros(x);
#endif
}

static ScreenSample PathTracer_renderPixel(PathTracer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 ix,
    const uint32 iy,
    const uniform uint32 accumID,
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

  PathContext pathContext;
  pathContext.context = self;
  pathContext.world = world;

  uniform PathTracerData *uniform ptData = world->pathtracerData;
  const uniform uint32 numLights = ptData->lights ? ptData->numLights : 0;

  const uniform uint32 numFirstBounceLightSamples =
      self->numLightSamples >= 0 && numLights > 0
      ? self->numLightSamples
      : min(MAX_LIGHT_SAMPLES, numLights);
  const uniform uint32 numIndirectBounceLightSamples =
      self->limitIndirectLightSamples && numFirstBounceLightSamples >= 1
      ? 1
      : numFirstBounceLightSamples;

  pathContext.numLights = numLights;
  pathContext.numGeoLights = ptData->numGeoLights;
  pathContext.numFirstBounceLightSamples = numFirstBounceLightSamples;
  pathContext.numIndirectBounceLightSamples = numIndirectBounceLightSamples;
  pathContext.lights = ptData->lights;
  pathContext.lightsCDF = ptData->lightsCDF;
#ifdef OSPRAY_PATHTRACER_DEBUG
  pathContext.disableNEE =
      numFirstBounceLightSamples == 0 && numIndirectBounceLightSamples == 0;
  pathContext.disableFWD = false;
  pathContext.debug = false;
#endif

  PathState pathState;

  const uniform int spp = self->super.spp;
  uint32 sampleOffs;
  uint32 sampleOffsLight;
  uniform uint32 sampleBits;
  uniform uint32 sampleBitsLight;
  uint32 seed = 0x49c5f37e; // fixed seed with random bits

  if (fb->targetFrames) {
    // blue noise: negatively correlate pixels by
    // - using a single sequence (identical seed for all pixels)
    // - split across the frame (2D-shuffled space-filling curve to select
    //   adjacent sub-sequences for neighboring pixels)
    const uniform uint32 targetSpp = fb->targetFrames * spp;
    // limit to 256x256 tiles, to have 16bits for 64k samples without overflow
    sampleOffs = OwenScramble4(interleaveZOrder16(ix, iy), seed) * targetSpp;
    // NEE splitting: further space sequence by max light samples
    sampleOffsLight = sampleOffs * numFirstBounceLightSamples;
    sampleBits = 16 + 32 - clz(targetSpp - 1);
    sampleBitsLight = 48 - clz(targetSpp * numFirstBounceLightSamples - 1);
  } else {
    // decorrelate pixels via different seed, whole sequence used for each pixel
    seed ^= fb->size.x * iy + ix; // avoid a zero for initializing the scramble
    sampleOffs = 0;
    sampleOffsLight = 0;
    sampleBits = 16;
    sampleBitsLight = 16;
  }

  for (uniform int s = 0; s < spp; s++) {
    const uint32 sampleID = accumID * spp + s;
    LDSampler_init(pathState.ldSampler,
        seed,
        sampleOffs + sampleID,
        sampleOffsLight + sampleID * numFirstBounceLightSamples,
        sampleBits,
        sampleBitsLight);
    RandomSampler_init(&pathState.randomSampler, seed, sampleOffs + sampleID);

    CameraSample cameraSample;
    cameraSample.pixel_center = (make_vec2f(ix, iy) + 0.5f) * fb->rcpSize;

    vec3ui ss234;
    const vec2f pixelSample =
        LDSampler_getNext5Samples(pathState.ldSampler, ss234);

    const PixelFilter *uniform pf = self->super.pixelFilter;
    const vec2f pfSample = pf ? PixelFilter_dispatch_sample(pf, pixelSample)
                              : pixelSample - make_vec2f(0.5f);

    cameraSample.screen = cameraSample.pixel_center + pfSample * fb->rcpSize;

    if (camera->needLensSample)
      cameraSample.lens = LDSampler_finalizeDim23(pathState.ldSampler, ss234);
    else
      cameraSample.lens = make_vec2f(0.5f);

    if (camera->needTimeSample)
      cameraSample.time = LDSampler_finalizeDim4(pathState.ldSampler, ss234);
    else
      cameraSample.time = 0.5f;

    Camera_dispatch_initRay(
        camera, screenSample.ray, screenSample.rayCone, cameraSample, ffh);
    // take screen resolution (unnormalized coordinates), i.e., pixel size into
    // account
    screenSample.rayCone.dwdt *= fb->rcpSize.y;
    screenSample.rayCone.width *= fb->rcpSize.y;

    const float tMax =
        Renderer_getMaxDepth(&self->super, cameraSample.screen, ffh);

    screenSample.ray.t = min(screenSample.ray.t, tMax);

    pathContext.screen = cameraSample.pixel_center;
    ScreenSample sample = PathTraceIntegrator_Li(
        pathContext, pathState, screenSample.ray, screenSample.rayCone, ffh);

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

inline void PathTracer_renderTask(const uniform vec3ui itemIndex,
    Renderer *uniform _self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const uniform FeatureFlagsHandler &ffh)
{
  PathTracer *uniform self = (PathTracer * uniform) _self;

  uniform RenderTaskDesc taskDesc =
      FrameBuffer_dispatch_getRenderTaskDesc(fb, taskIDs[itemIndex.z], ffh);

  if (fb->cancelRender || isEmpty(taskDesc.region)) {
    return;
  }

#ifndef ISPC
  {
    int32 y = taskDesc.region.lower.y + itemIndex.y;
    int32 x = taskDesc.region.lower.x + itemIndex.x;
#else
  foreach_tiled (y = taskDesc.region.lower.y... taskDesc.region.upper.y,
      x = taskDesc.region.lower.x... taskDesc.region.upper.x) {
#endif
    ScreenSample screenSample =
        PathTracer_renderPixel(self, fb, camera, world, x, y, fb->frameID, ffh);

    FrameBuffer_dispatch_accumulateSample(fb, screenSample, taskDesc, ffh);
  }
  FrameBuffer_dispatch_completeTask(fb, taskDesc, ffh);
}

// Exports (called from C++) //////////////////////////////////////////////////

DEFINE_RENDERER_KERNEL_LAUNCHER(PathTracer_renderTask);

OSPRAY_END_ISPC_NAMESPACE
