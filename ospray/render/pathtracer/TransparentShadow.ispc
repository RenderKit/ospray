// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/pathtracer/PathTracer.ih"
#include "render/pathtracer/TransparentShadow.ih"

#include "render/pathtracer/materials/Material.ih"
#include "render/pathtracer/materials/Medium.ih"

#include "common/Ray.ih"
#include "common/World.ih"

vec3f transparentShadow(const uniform PathTracer *uniform self,
    const World *uniform world,
    vec3f lightContrib,
    Ray &shadowRay,
    Medium medium)
{
  uniform int maxDepth = self->super.maxDepth;
  const float tOriginal = shadowRay.t;

  while (1) {
    traceRay(world, shadowRay);

    if (noHit(shadowRay))
      return lightContrib;

    DifferentialGeometry dg;
    postIntersect(world,
        &self->super,
        dg,
        shadowRay,
        DG_NS | DG_NG | DG_FACEFORWARD | DG_NORMALIZE | DG_TEXCOORD | DG_COLOR);

    uniform PathTraceMaterial *material =
        (uniform PathTraceMaterial *)dg.material;
    vec3f transparency;
    foreach_unique (m in material)
      if (m != NULL)
        transparency = m->getTransparency(m, dg, shadowRay, medium);

    lightContrib = lightContrib * transparency;

    // compute attenuation with Beer's law
    if (ne(medium.attenuation, 0.f))
      lightContrib = lightContrib
          * expf(medium.attenuation * (shadowRay.t - shadowRay.t0));

    if (reduce_max(lightContrib) <= self->super.minContribution)
      return lightContrib;

    if (--maxDepth <= 0)
      return make_vec3f(0.f);

    /*! Tracking medium if we hit a medium interface. */
    foreach_unique (m in material)
      if (m != NULL)
        m->selectNextMedium(m, dg, medium);

    shadowRay.t0 = shadowRay.t + dg.epsilon;
    shadowRay.t = tOriginal;
    shadowRay.primID = -1;
    shadowRay.geomID = -1;
    shadowRay.instID = -1;
  }
}
