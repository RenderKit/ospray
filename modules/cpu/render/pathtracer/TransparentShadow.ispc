// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/TransparentShadow.ih"

#include "render/Material.ih"
#include "render/MaterialDispatch.ih"
#include "render/materials/Medium.ih"

#include "common/Ray.ih"
#include "common/World.ih"

// c++ shared
#include "PathTracerShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline vec3f transparentShadow(
    const uniform PathTracer *uniform self,
    const World *uniform world,
    vec3f lightContrib,
    Ray &shadowRay,
    RayCone &shadowCone,
    RayIntervals &rayIntervals,
    Medium medium,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  // It's not possible to have transparent shadows if we don't have any geometry
  if (!ff.geometry) {
    return lightContrib;
  }

  uniform uint32 maxDepth = self->super.maxDepth;
  const float tOriginal = shadowRay.t;
  const float widthOriginal = shadowCone.width;

  while (1) {
    traceGeometryRayIntervals(world, shadowRay, rayIntervals, ffh);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(world,
        &self->super,
        dg,
        shadowRay,
        shadowCone,
        DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR
            | DG_MOTIONBLUR,
        ffh);

    uniform Material *material = (uniform Material *)dg.material;
    vec3f transparency;

    foreach_unique (m in material) {
      if (m != NULL) {
        transparency =
            Material_dispatch_getTransparency(m, dg, shadowRay, medium, ffh);
      }
    }

    lightContrib = lightContrib * transparency;

    // compute attenuation with Beer's law
    if (ne(medium.attenuation, 0.f))
      lightContrib = lightContrib
          * expf(medium.attenuation * (shadowRay.t - shadowRay.t0));

    if (reduce_max(lightContrib) <= self->super.minContribution
        || --maxDepth <= 0)
      return make_vec3f(0.f);

    // Tracking medium if we hit a medium interface.
    foreach_unique (m in material) {
      if (m != NULL) {
        Material_dispatch_selectNextMedium(m, dg, medium, ffh);
      }
    }

    setRay(shadowRay, shadowRay.t + dg.epsilon, tOriginal);
    shadowCone.width = widthOriginal;
  }
}

OSPRAY_END_ISPC_NAMESPACE
