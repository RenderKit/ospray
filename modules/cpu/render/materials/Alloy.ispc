// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "texture/TextureParam.ih"

#include "AlloyShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Alloy_getBSDF(const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Alloy *uniform self = (const uniform Alloy *uniform)super;
  varying linear3f *uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f color = self->color * make_vec3f(dg.color)
      * get3f(self->colorMap, dg, make_vec3f(1.f));

  const vec3f edgeColor =
      self->edgeColor * get3f(self->edgeColorMap, dg, make_vec3f(1.f));

  Fresnel *uniform fresnel = FresnelSchlick_create(ctx, color, edgeColor);

  const float roughness = self->roughness * get1f(self->roughnessMap, dg, 1.f);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, fresnel);
  else
    return MicrofacetConductor_create(ctx, frame, fresnel, roughness, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Alloy_getBSDF_addr()
{
  return Alloy_getBSDF;
}
