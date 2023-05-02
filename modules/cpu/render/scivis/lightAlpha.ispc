// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/FeatureFlagsEnum.h"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
// SciVis renderer
#include "surfaces.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "render/ao/volumes.ih"
#endif
// c++ shared
#include "SciVisShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

// Lighting functions //
SYCL_EXTERNAL vec3f lightAlpha(const uniform SciVis *uniform self,
    Ray &ray,
    const World *uniform world,
    varying LDSampler *uniform ldSampler,
    vec3f weight,
    uniform float quality,
    const uniform FeatureFlagsHandler &ffh)
{
  vec3f alpha = make_vec3f(1.f);
  const float org_t_max = ray.t;

  const uniform FeatureFlags ff = getFeatureFlags(ffh);

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

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
  traceClippingRay(world, ray, rayIntervals, ffh);

  while (true) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    // WA for https://jira.devtools.intel.com/browse/XDEPS-4875
#ifndef OSPRAY_TARGET_SYCL
    traceGeometryRayIntervals(world, ray, rayIntervals, ffh);
#else
    traceGeometryRay(world, ray, ffh);
#endif

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
            self->volumeSamplingRate * quality,
            ffh);

        alpha = alpha * make_vec3f(1.f - volumeColor.w);
      }
    }
#endif

    // If any geometry has been hit
    const bool rayHadHit = hadHit(ray);
    if (rayHadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      postIntersect(world, &self->super, dg, ray, DG_COLOR | DG_TEXCOORD, ffh);

      const SciVisBSDF bsdf = evalMaterial(dg, ffh);
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

#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  // No dynamic allocation on GPU
  freeVolumeIntervals(volumeIntervals);
#endif
  return alpha;
}

OSPRAY_END_ISPC_NAMESPACE
