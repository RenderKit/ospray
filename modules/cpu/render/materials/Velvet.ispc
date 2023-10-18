// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Velvet.ih"

#include "render/Material.ih"
#include "render/bsdfs/Minneart.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/Velvety.ih"
// c++ shared
#include "VelvetShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation
OSPRAY_BEGIN_ISPC_NAMESPACE

#define MULTIBSDF_SIZE 2
struct Velvet_BSDF
{
  DEFINE_MULTIBSDF(MULTIBSDF_SIZE);
  Minneart minneart;
  Velvety velvety;
};

SYCL_EXTERNAL const varying BSDF *uniform Velvet_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform Velvet *uniform self = (const uniform Velvet *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));

  // Allocate memory and initialize material BSDF
  varying Velvet_BSDF *uniform bsdf = (varying Velvet_BSDF * uniform)
      ShadingContext_alloc(ctx, sizeof(Velvet_BSDF));
  varying MultiBSDF *uniform mbsdf = (varying MultiBSDF * uniform) bsdf;
  MultiBSDF_Constructor(mbsdf, MULTIBSDF_SIZE);
  bsdf->super.bsdfType = BSDF_TYPE_VELVET;

  Minneart_Constructor(
      &bsdf->minneart, shadingFrame, self->reflectance, self->backScattering);
  MultiBSDF_add(
      mbsdf, 0, &bsdf->minneart.super, 1.f, luminance(self->reflectance));

  Velvety_Constructor(&bsdf->velvety,
      shadingFrame,
      self->horizonScatteringColor,
      self->horizonScatteringFallOff);
  MultiBSDF_add(mbsdf,
      1,
      &bsdf->velvety.super,
      1.f,
      luminance(self->horizonScatteringColor));

  return &bsdf->super;
}

SYCL_EXTERNAL BSDF_EvalRes Velvet_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying Velvet_BSDF *uniform self =
      (const varying Velvet_BSDF *uniform)super;

  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, &self->minneart, Minneart_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->velvety, Velvety_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

SYCL_EXTERNAL BSDF_SampleRes Velvet_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying Velvet_BSDF *uniform self =
      (const varying Velvet_BSDF *uniform)super;

  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->minneart, Minneart_sample);
  MULTIBSDF_SAMPLE_CHILD(1, &self->velvety, Velvety_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, &self->minneart, Minneart_eval);
  MULTIBSDF_EVAL_CHILD(1, &self->velvety, Velvety_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Velvet_getBSDF_addr()
{
  return (void *uniform)Velvet_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
