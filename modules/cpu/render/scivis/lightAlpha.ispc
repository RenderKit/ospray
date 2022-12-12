// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
// SciVis renderer
#include "surfaces.ih"
// TODO: Volume intervals/TLS/etc needs more redesign for GPU
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
#include "volumes.ih"
#endif
// c++ shared
#include "SciVisShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

// Lighting functions //
SYCL_EXTERNAL vec3f lightAlpha(const uniform SciVis *uniform self,
    Ray &ray,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    ScreenSample &sample,
    varying LDSampler *uniform ldSampler,
    vec3f weight,
    float rayOffset,
    uniform float quality)
{
  vec3f alpha = make_vec3f(1.f);
  const float org_t_max = ray.t;

  // TODO: Volume intervals/TLS/etc needs more redesign for GPU
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  // Allocate memory for volume intervals
  VolumeIntervals volumeIntervals;
  allocVolumeIntervals(volumeIntervals);
#endif
  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
#ifndef OSPRAY_TARGET_SYCL
  traceClippingRay(world, ray, rayIntervals);
#endif

  while (true) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    // WA for https://jira.devtools.intel.com/browse/XDEPS-4875
#ifndef OSPRAY_TARGET_SYCL
    traceGeometryRayIntervals(world, ray, rayIntervals);
#else
    traceGeometryRay(world, ray);
#endif

    // TODO: Volume intervals/TLS/etc needs more redesign for GPU
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
    // Determine volume intervals by tracing ray in the volume scene
    Ray volumeRay = ray;
    traceVolumeRay(world, volumeRay, volumeIntervals);

    // Sample volumes across volume intervals (in front of geometry hit)
    if (volumeIntervals.numVolumeIntervals > 0) {
      SciVisRenderContext rc;
      rc.renderer = self;
      rc.fb = fb;
      rc.world = world;
      rc.sample = sample;
      rc.ldSampler = ldSampler;
      vec4f volumeColor = integrateVolumeIntervalsGradient(rc,
          volumeIntervals,
          rayIntervals,
          volumeRay,
          ldSampler,
          self->volumeSamplingRate * quality,
          false);

      alpha = alpha * make_vec3f(volumeColor.w);
    }
#endif

    // If any geometry has been hit
    const bool rayHadHit = hadHit(ray);
    if (rayHadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      postIntersect(world, &self->super, dg, ray, DG_COLOR | DG_TEXCOORD);

      const SciVisBSDF bsdf = evalMaterial(dg);
      alpha = alpha * bsdf.transmission;

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, org_t_max);
    } else {
      break;
    }

    // Check if there is enough contribution from this light
    if (luminance(alpha * weight) < self->super.minContribution) {
      alpha = make_vec3f(0.f);
      break;
    }
  }

  // TODO: Volume intervals/TLS/etc needs more redesign for GPU
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  freeVolumeIntervals(volumeIntervals);
#endif
  return alpha;
}

OSPRAY_END_ISPC_NAMESPACE
