// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Medium.ih"
#include "render/Material.ih"
#include "render/bsdfs/RobustDielectric.ih"

#include "GlassShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Glass_getBSDF(const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  uniform const Glass *uniform self = (uniform const Glass *uniform)super;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  varying BSDF *uniform bsdf = RobustDielectric_create(ctx, dg, eta);
  return bsdf;
}

vec3f Glass_getTransparency(const uniform Material *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)material;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f(1.0f - fresnelDielectric(cosThetaO, eta));
}

void Glass_selectNextMedium(const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)super;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Glass_getBSDF_addr()
{
  return Glass_getBSDF;
}

export void *uniform Glass_getTransparency_addr()
{
  return Glass_getTransparency;
}

export void *uniform Glass_selectNextMedium_addr()
{
  return Glass_selectNextMedium;
}