// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "render/Renderer.ih"
// AO renderer
#include "surfaces.ih"
#include "volumes.ih"
// c++ shared
#include "AORendererShared.h"
#include "fb/FrameBufferShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

RTC_SYCL_INDIRECTLY_CALLABLE void AORenderer_renderSample(
    Renderer *uniform _self,
    FrameBuffer *uniform fb,
    World *uniform world,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  uniform AORenderer *uniform self = (uniform AORenderer * uniform) _self;

  LDSampler ldSamplerObj;
  varying LDSampler *uniform ldSampler = &ldSamplerObj;
  LDSampler_init(ldSampler,
      self->super.mathConstants,
      sample.sampleID.x + fb->size.x * sample.sampleID.y,
      sample.sampleID.z);

  uniform bool firstHit = true;
  const float originalRayTFar = sample.ray.t;

  // Allocate memory for volume intervals
#ifdef OSPRAY_ENABLE_VOLUMES
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif

  // This is our main ray
  Ray &ray = sample.ray;

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
#ifndef OSPRAY_TARGET_SYCL
  traceClippingRay(world, ray, rayIntervals);
#else
  rayIntervals.count = 1;
  rayIntervals.intervals[0].lower = ray.t0;
  rayIntervals.intervals[0].upper = ray.t;
#endif

  // Iterate over all translucent geometry till we are fully opaque
  vec4f outputColor = make_vec4f(0.f);
  while (outputColor.w < 0.99f) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    // Note: work around for bug when using traceGeometryRayIntervals on GPU
#ifdef OSPRAY_TARGET_SYCL
    traceGeometryRay(world, ray);
#else
    traceGeometryRayIntervals(world, ray, rayIntervals);
#endif

#ifdef OSPRAY_ENABLE_VOLUMES
    // Determine volume intervals by tracing ray in the volume scene
    Ray volumeRay = ray;
    traceVolumeRay(world, volumeRay, volumeIntervals);

    // Sample volumes across volume intervals (in front of geometry hit)
    if (volumeIntervals.numVolumeIntervals > 0) {
      vec4f volumeColor = integrateVolumeIntervals(volumeIntervals,
          rayIntervals,
          volumeRay,
          ldSampler,
          self->volumeSamplingRate);
      outputColor = outputColor + (1.f - outputColor.w) * volumeColor;
    }
#endif

    // If any geometry has been hit
    vec4f blendedColor;
    const bool rayHadHit = hadHit(ray);
    if (rayHadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      computeDG(world, self, ray, dg);

      // Shade geometry
      SSI surfaceShading;
      surfaceShading = AORenderer_computeShading(self, fb, world, dg, sample);

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
      blendedColor = Renderer_getBackground(&self->super, sample.pos);
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

#ifdef OSPRAY_ENABLE_VOLUMES
  freeVolumeIntervals(volumeIntervals);
#endif
  sample.rgb = make_vec3f(outputColor);
  sample.alpha = outputColor.w;
}

// AORenderer C++ interface ////////////////////////////////////////////////////

export void *uniform AORenderer_renderSample_addr()
{
  return (void *uniform)AORenderer_renderSample;
}

OSPRAY_END_ISPC_NAMESPACE
