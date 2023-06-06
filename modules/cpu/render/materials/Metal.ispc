// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Metal.ih"

#include "math/spectrum.ih"
#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "MetalShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation
OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL const varying BSDF *uniform Metal_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform Metal *uniform self = (const uniform Metal *uniform)super;
  varying linear3f *uniform shadingframe =
      LinearSpace3f_create(ctx, frame(dg.Ns));

  Fresnel *uniform fresnel;
  if (self->spectral)
    fresnel = FresnelConductorSpectral_create(ctx, self->eta, self->k);
  else
    fresnel = FresnelConductorRGBUniform_create(ctx, self->etaRGB, self->kRGB);

  varying BSDF *uniform bsdf;
  if (self->roughness == 0.0f) {
    bsdf = Conductor_create(ctx, shadingframe, fresnel);
    bsdf->bsdfType = BSDF_TYPE_METAL;
  } else {
    bsdf = MicrofacetConductor_create(ctx,
        super->microfacetAlbedoTables,
        shadingframe,
        fresnel,
        self->roughness * get1f(self->roughnessMap, dg, 1.f),
        0.f);
    bsdf->bsdfType = BSDF_TYPE_METAL_ROUGH;
  }
  return bsdf;
}

SYCL_EXTERNAL BSDF_EvalRes MetalRough_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  return MicrofacetConductor_eval(
      (const varying MicrofacetConductor *uniform)super, wo, wi);
}

SYCL_EXTERNAL BSDF_SampleRes MetalRough_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return MicrofacetConductor_sample(
      (const varying MicrofacetConductor *uniform)super, wo, s, ss);
}

SYCL_EXTERNAL BSDF_SampleRes Metal_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return Conductor_sample(super, wo, s, ss);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Metal_getBSDF_addr()
{
  return (void *uniform)Metal_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
