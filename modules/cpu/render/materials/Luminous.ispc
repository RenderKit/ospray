// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Luminous.ih"

#include "render/Material.ih"
#include "render/bsdfs/Transmission.ih"
// c++ shared
#include "LuminousShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL const varying BSDF *uniform Luminous_getBSDF(
    const uniform Material *uniform _self,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  uniform Luminous *uniform self = (uniform Luminous * uniform) _self;
  varying BSDF *uniform bsdf = NULL;

  if (self->transparency > 0.0f) {
    vec3f T = make_vec3f(self->transparency);
    bsdf = Transmission_create(ctx, NULL /*no frame needed*/, T);
    bsdf->bsdfType = BSDF_TYPE_LUMINOUS;
  }

  return bsdf;
}

SYCL_EXTERNAL vec3f Luminous_getTransparency(
    const uniform Material *uniform _self,
    const DifferentialGeometry &,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  uniform Luminous *uniform self = (uniform Luminous * uniform) _self;

  return make_vec3f(self->transparency);
}

SYCL_EXTERNAL BSDF_SampleRes Luminous_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return Transmission_sample(super, wo, s, ss);
}

SYCL_EXTERNAL vec3f Luminous_getEmission(const Material *uniform _self,
    const varying DifferentialGeometry &dg,
    const uniform FeatureFlagsHandler &)
{
  const Luminous *uniform self = (Luminous * uniform) _self;

  return make_vec3f(self->radiance)
      * get3f(self->emissionMap, dg, make_vec3f(1.f));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Luminous_getBSDF_addr()
{
  return (void *uniform)Luminous_getBSDF;
}

export void *uniform Luminous_getTransparency_addr()
{
  return (void *uniform)Luminous_getTransparency;
}

export void *uniform Luminous_getEmission_addr()
{
  return (void *uniform)Luminous_getEmission;
}

OSPRAY_END_ISPC_NAMESPACE
