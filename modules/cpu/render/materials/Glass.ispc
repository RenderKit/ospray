// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Glass.ih"

#include "Medium.ih"
#include "common/Ray.ih"
#include "render/Material.ih"
#include "render/bsdfs/RobustDielectric.ih"
// c++ shared
#include "GlassShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL const varying BSDF *uniform Glass_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &)
{
  uniform const Glass *uniform self = (uniform const Glass *uniform)super;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  varying BSDF *uniform bsdf = RobustDielectric_create(ctx, dg, eta);
  bsdf->bsdfType = BSDF_TYPE_GLASS;
  return bsdf;
}

SYCL_EXTERNAL vec3f Glass_getTransparency(
    const uniform Material *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)material;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f(1.0f - fresnelDielectric(cosThetaO, eta));
}

SYCL_EXTERNAL void Glass_selectNextMedium(const uniform Material *uniform super,
    const DifferentialGeometry &,
    Medium &currentMedium)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)super;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

SYCL_EXTERNAL BSDF_SampleRes Glass_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return RobustDielectric_sample(super, wo, s, ss);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Glass_getBSDF_addr()
{
  return (void *uniform)Glass_getBSDF;
}

export void *uniform Glass_getTransparency_addr()
{
  return (void *uniform)Glass_getTransparency;
}

export void *uniform Glass_selectNextMedium_addr()
{
  return (void *uniform)Glass_selectNextMedium;
}

OSPRAY_END_ISPC_NAMESPACE
