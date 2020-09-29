// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "lights/Light.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"
// SciVis renderer
#include "SciVisMaterial.ih"
#include "surfaces.ih"
#include "volumes.ih"

// Lighting functions //
float lightAlpha(const uniform SciVis *uniform self,
    Ray &ray,
    const FrameBuffer *uniform fb,
    const World *uniform world,
    ScreenSample &sample,
    varying LDSampler *uniform ldSampler,
    float weight,
    float rayOffset,
    uniform float quality)
{
  float alpha = 1.f;
  const float org_t_max = ray.t;

  // First trace the ray across clipping scene to calculate ray intervals,
  // this step should keep ray structure unchanged
  RayIntervals rayIntervals;
  traceClippingRay(world, ray, rayIntervals);

  while (1) {
    // Then trace normal geometry using calculated ray intervals,
    // if hit ray.t will be updated
    traceGeometryRayIntervals(world, ray, rayIntervals);

    // Determine volume intervals by tracing ray in the volume scene
    Ray volumeRay = ray;
    VolumeIntervals volumeIntervals;
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

      alpha *= 1.f - volumeColor.w;
    }

    // If any geometry has been hit
    vec4f blendedColor;
    const bool hadHit = hadHit(ray);
    if (hadHit) {
      // Prepare differential geometry structure
      DifferentialGeometry dg;
      postIntersect(world, &self->super, dg, ray, DG_COLOR | DG_TEXCOORD);

      const SciVisBSDF bsdf = evalMaterial(dg);

      alpha *= 1.f - bsdf.opacity;

      // Prepare ray for next loop iteration,
      // start from the last geometry hit all over to initial Tfar
      setRay(ray, ray.t + dg.epsilon, org_t_max);
    } else
      return alpha;

    if (alpha * weight < 0.03f)
      return 0.f;
  }
}
