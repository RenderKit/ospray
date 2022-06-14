// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/spectrum.ih"
#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "texture/TextureParam.ih"

#include "MetalShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Metal_getBSDF(const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Metal *uniform self = (const uniform Metal *uniform)super;
  varying linear3f *uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  Fresnel *uniform fresnel;
  if (self->spectral)
    fresnel = FresnelConductorSpectral_create(ctx, self->eta, self->k);
  else
    fresnel = FresnelConductorRGBUniform_create(ctx, self->etaRGB, self->kRGB);

  const float roughness = self->roughness * get1f(self->roughnessMap, dg, 1.f);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, fresnel);
  else
    return MicrofacetConductor_create(ctx, frame, fresnel, roughness, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Metal_getBSDF_addr()
{
  return Metal_getBSDF;
}
