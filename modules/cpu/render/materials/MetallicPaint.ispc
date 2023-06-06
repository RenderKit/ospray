// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MetallicPaint.ih"

#include "render/Material.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "MetallicPaintShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

struct MetallicPaint_MultiBSDF
{
  DEFINE_MULTIBSDF(2);
  BSDF lambert;
  MicrofacetConductor microfacetConductor;
};

struct MetallicPaint_BSDF
{
  DielectricLayer dielectricLayer;
  MetallicPaint_MultiBSDF multi;
};

SYCL_EXTERNAL const varying BSDF *uniform MetallicPaint_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform MetallicPaint *uniform self =
      (const uniform MetallicPaint *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));

  // Allocate memory and initialize material BSDF
  varying MetallicPaint_BSDF *uniform bsdf =
      (varying MetallicPaint_BSDF * uniform)
          ShadingContext_alloc(ctx, sizeof(MetallicPaint_BSDF));
  varying MultiBSDF *uniform bsdfMulti =
      (varying MultiBSDF * uniform) & bsdf->multi;
  MultiBSDF_Constructor(bsdfMulti, 2);

  const vec3f color = self->baseColor * make_vec3f(dg.color)
      * get3f(self->baseColorMap, dg, make_vec3f(1.f));
  Lambert_Constructor(&bsdf->multi.lambert, shadingFrame, color);
  MultiBSDF_add(bsdfMulti, 0, &bsdf->multi.lambert, 1.f, luminance(color));

  if (self->flakeAmount > 0.f) {
    const vec3f r = self->flakeColor;
    const vec3f g = make_vec3f(self->flakeAmount);
    Fresnel *uniform fresnel = FresnelSchlick_create(ctx, r, g);
    MicrofacetConductor_Constructor(&bsdf->multi.microfacetConductor,
        super->microfacetAlbedoTables,
        shadingFrame,
        fresnel,
        self->flakeSpread,
        0.f);
    MultiBSDF_add(bsdfMulti,
        1,
        (varying BSDF * uniform) & bsdf->multi.microfacetConductor,
        1.f,
        luminance(r));
  }

  DielectricLayer_Constructor(&bsdf->dielectricLayer,
      shadingFrame,
      &bsdf->multi.super,
      self->eta,
      make_vec3f(1.0f),
      1.0f,
      1.0f);
  bsdf->dielectricLayer.super.bsdfType = BSDF_TYPE_METALLICPAINT;
  return &bsdf->dielectricLayer.super;
}

inline BSDF_EvalRes MultiBSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying MetallicPaint_MultiBSDF *uniform self =
      (const varying MetallicPaint_MultiBSDF *uniform)super;

  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, Lambert_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->microfacetConductor, MicrofacetConductor_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

inline BSDF_SampleRes MultiBSDF_sample(const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying MetallicPaint_MultiBSDF *uniform self =
      (const varying MetallicPaint_MultiBSDF *uniform)super;

  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->lambert, Lambert_sample);
  MULTIBSDF_SAMPLE_CHILD(
      1, &self->microfacetConductor, MicrofacetConductor_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, Lambert_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->microfacetConductor, MicrofacetConductor_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

SYCL_EXTERNAL BSDF_EvalRes MetallicPaint_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying MetallicPaint_BSDF *uniform self =
      (const varying MetallicPaint_BSDF *uniform)super;

  DIELECTRICLAYER_EVAL(self->dielectricLayer,
      self->multi.super.scatteringType,
      &self->multi.super,
      MultiBSDF_eval);
  return DIELECTRICLAYER_EVAL_GET();
}

SYCL_EXTERNAL BSDF_SampleRes MetallicPaint_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying MetallicPaint_BSDF *uniform self =
      (const varying MetallicPaint_BSDF *uniform)super;

  DIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      self->multi.super.scatteringType,
      &self->multi.super,
      MultiBSDF_sample);
  return DIELECTRICLAYER_SAMPLE_GET();
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform MetallicPaint_getBSDF_addr()
{
  return (void *uniform)MetallicPaint_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
