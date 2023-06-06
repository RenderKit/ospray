// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Plastic.ih"

#include "render/Material.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
// c++ shared
#include "PlasticShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

struct Plastic_BSDF
{
  BSDF root;
  MicrofacetDielectricLayer dielectricLayer;
  BSDF lambert;
};

SYCL_EXTERNAL const varying BSDF *uniform Plastic_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform Plastic *uniform self = (const uniform Plastic *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));

  // Allocate memory and initialize material BSDF
  varying Plastic_BSDF *uniform bsdf = (varying Plastic_BSDF * uniform)
      ShadingContext_alloc(ctx, sizeof(Plastic_BSDF));
  const vec3f _pigmentColor = self->pigmentColor;
  Lambert_Constructor(&bsdf->lambert, shadingFrame, _pigmentColor);

  if (self->roughness == 0.0f) {
    DielectricLayer_Constructor(
        (varying DielectricLayer * uniform) & bsdf->dielectricLayer,
        shadingFrame,
        &bsdf->lambert,
        self->eta,
        make_vec3f(1.0f),
        1.0f,
        1.0f);
  } else {
    MicrofacetDielectricLayer_Constructor(&bsdf->dielectricLayer,
        super->microfacetAlbedoTables,
        shadingFrame,
        &bsdf->lambert,
        self->eta,
        make_vec3f(1.0f),
        1.0f,
        self->roughness,
        0.f,
        1.0f);
  }

  bsdf->root = bsdf->dielectricLayer.super;
  bsdf->root.bsdfType = BSDF_TYPE_PLASTIC;
  return &bsdf->root;
}

// DielectricLayer BSDF

inline BSDF_EvalRes DielectricLayerBSDF_eval(
    const varying Plastic_BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  DIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->lambert.scatteringType,
      &self->lambert,
      Lambert_eval);
  return DIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes DielectricLayerBSDF_sample(
    const varying Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  DIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->lambert.scatteringType,
      &self->lambert,
      Lambert_sample);
  return DIELECTRICLAYER_SAMPLE_GET();
}

// MicrofacetDielectricLayer BSDF

inline BSDF_EvalRes MicrofacetDielectricLayerBSDF_eval(
    const varying Plastic_BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  MICROFACETDIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->lambert.scatteringType,
      &self->lambert,
      Lambert_eval);
  return MICROFACETDIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes MicrofacetDielectricLayerBSDF_sample(
    const varying Plastic_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  MICROFACETDIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->lambert.scatteringType,
      &self->lambert,
      Lambert_eval,
      Lambert_sample);
  return MICROFACETDIELECTRICLAYER_SAMPLE_GET();
}

// Plastic BSDF

SYCL_EXTERNAL __noinline BSDF_EvalRes Plastic_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying Plastic_BSDF *uniform self =
      (const varying Plastic_BSDF *uniform)super;

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return MicrofacetDielectricLayerBSDF_eval(self, wo, wi);
  else
    return DielectricLayerBSDF_eval(self, wo, wi);
}

SYCL_EXTERNAL __noinline BSDF_SampleRes Plastic_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying Plastic_BSDF *uniform self =
      (const varying Plastic_BSDF *uniform)super;

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return MicrofacetDielectricLayerBSDF_sample(self, wo, s, ss);
  else
    return DielectricLayerBSDF_sample(self, wo, s, ss);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Plastic_getBSDF_addr()
{
  return (void *uniform)Plastic_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
