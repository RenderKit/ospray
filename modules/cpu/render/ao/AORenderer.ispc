// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "camera/Camera.ih"
#include "camera/CameraDispatch.ih"
#include "common/FeatureFlagsEnum.h"
#include "fb/FrameBuffer.ih"
#include "fb/FrameBufferDispatch.ih"
#include "fb/RenderTaskDesc.ih"
#include "math/random.ih"
#include "pf/PixelFilterDispatch.ih"
#include "render/Renderer.ih"
#include "render/ScreenSample.ih"
// AO renderer
#include "surfaces.ih"
#include "volumes.ih"
// c++ shared
#include "AORendererShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

static void AORenderer_renderSample(Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    varying ScreenSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  uniform AORenderer *uniform self = (uniform AORenderer * uniform) _self;

  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      self->super.mathConstants,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  uniform bool firstHit = true;
  const float originalRayTFar = sample.ray.t;

#ifdef OSPRAY_ENABLE_VOLUMES
#ifdef OSPRAY_TARGET_SYCL
  // We only support a single volume interval on the GPU
  VolumeInterval vInterval;
  VolumeIntervals volumeIntervals;
  volumeIntervals.numVolumeIntervals = 0;
  volumeIntervals.numAllocated = 1;
  volumeIntervals.intervals = &vInterval;
#else
  // Allocate memory for volume intervals
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif
#endif

  // This is our main ray
  Ray &ray = sample.ray;

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
  traceClippingRay(world, ray, rayIntervals, ffh);

  // Iterate over all translucent geometry till we are fully opaque
  vec4f outputColor = make_vec4f(0.f);
  while (outputColor.w < 0.99f) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    if (ff.geometry) {
      traceGeometryRayIntervals(world, ray, rayIntervals, ffh);
    }

#ifdef OSPRAY_ENABLE_VOLUMES
    if (ff.other & FFO_VOLUME_IN_SCENE) {
      // Determine volume intervals by tracing ray in the volume scene
      Ray volumeRay = ray;
      traceVolumeRay(world, volumeRay, volumeIntervals);

      // Sample volumes across volume intervals (in front of geometry hit)
      if (volumeIntervals.numVolumeIntervals > 0) {
        vec4f volumeColor = integrateVolumeIntervals(volumeIntervals,
            rayIntervals,
            volumeRay,
            ldSampler,
            self->volumeSamplingRate,
            ffh);
        outputColor = outputColor + (1.f - outputColor.w) * volumeColor;
      }
    }
#endif

    // If any geometry has been hit
    vec4f blendedColor;
    const bool rayHadHit = hadHit(ray);
    if (rayHadHit && ff.geometry) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      computeDG(world, self, ray, dg, ffh);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = AORenderer_computeShading(self, world, dg, sample, ffh);

      // Use shaded color for blending
      blendedColor = surfaceShading.shadedColor;

      // Initialize other per sample data with first hit values
      if (firstHit) {
        sample.z = ray.t;
        sample.albedo = surfaceShading.albedo;
        sample.normal = dg.Ns;
        sample.instID = dg.instID;
        sample.geomID = dg.objID;
        sample.primID = ray.primID;
      }

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, originalRayTFar);
    } else {
      blendedColor = Renderer_getBackground(&self->super, sample.pos, ffh);
      // Initialize other per sample data with first hit values
      if (firstHit) {
        sample.z = ray.t;
        sample.albedo = make_vec3f(blendedColor);
        sample.normal = sample.ray.dir;
      }
    }

    // Blend with output final color
    outputColor = outputColor + (1.f - outputColor.w) * blendedColor;
    firstHit = false;
    if (!rayHadHit) {
      break;
    }
  }

#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  freeVolumeIntervals(volumeIntervals);
#endif
  sample.rgb = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

#define renderSampleFn AORenderer_renderSample
#include "render/RendererRenderTaskFn.inl"
#undef renderSampleFn

#ifdef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL void AORenderer_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh)
{
  Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, taskIndex0, ffh);
}
#else
export void AORenderer_renderTasks(void *uniform _self,
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
  launch[numTasks] Renderer_default_renderTask(
      self, fb, camera, world, taskIDs, ffh);
  sync;
}
#endif

OSPRAY_END_ISPC_NAMESPACE
