// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Alloy.ih"

#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "AlloyShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL const varying BSDF *uniform Alloy_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform Alloy *uniform self = (const uniform Alloy *uniform)super;
  varying linear3f *uniform shadingframe =
      LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f color = self->color * make_vec3f(dg.color)
      * get3f(self->colorMap, dg, make_vec3f(1.f));

  const vec3f edgeColor =
      self->edgeColor * get3f(self->edgeColorMap, dg, make_vec3f(1.f));

  Fresnel *uniform fresnel = FresnelSchlick_create(ctx, color, edgeColor);

  varying BSDF *uniform bsdf;
  if (self->roughness == 0.0f) {
    bsdf = Conductor_create(ctx, shadingframe, fresnel);
    bsdf->bsdfType = BSDF_TYPE_ALLOY;
  } else {
    bsdf = MicrofacetConductor_create(ctx,
        super->microfacetAlbedoTables,
        shadingframe,
        fresnel,
        self->roughness * get1f(self->roughnessMap, dg, 1.f),
        0.f);
    bsdf->bsdfType = BSDF_TYPE_ALLOY_ROUGH;
  }
  return bsdf;
}

SYCL_EXTERNAL BSDF_EvalRes AlloyRough_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  return MicrofacetConductor_eval(
      (const varying MicrofacetConductor *uniform)super, wo, wi);
}

SYCL_EXTERNAL BSDF_SampleRes AlloyRough_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return MicrofacetConductor_sample(
      (const varying MicrofacetConductor *uniform)super, wo, s, ss);
}

SYCL_EXTERNAL BSDF_SampleRes Alloy_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  return Conductor_sample(super, wo, s, ss);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Alloy_getBSDF_addr()
{
  return (void *uniform)Alloy_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
