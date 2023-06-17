// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/Transmission.ih"

#include "Luminous.ih"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL const varying BSDF *varying Luminous_getBSDF(
    const uniform Material *uniform _self,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &,
    const Ray &,
    const Medium &)
{
  uniform Luminous *uniform self = (uniform Luminous * uniform) _self;
  varying BSDF *uniform bsdf = NULL;

  if (self->transparency > 0.0f) {
    vec3f T = make_vec3f(self->transparency);
    bsdf = Transmission_create(ctx, NULL /*no frame needed*/, T);
  }

  return bsdf;
}

SYCL_EXTERNAL vec3f Luminous_getTransparency(
    const uniform Material *uniform _self,
    const DifferentialGeometry &,
    const Ray &,
    const Medium &)
{
  uniform Luminous *uniform self = (uniform Luminous * uniform) _self;

  return make_vec3f(self->transparency);
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

OSPRAY_END_ISPC_NAMESPACE
