// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Mix.ih"

#include "common/FeatureFlags.ih"
#include "render/Material.ih"
#include "render/MaterialDispatch.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "texture/TextureParam.ih"

// c++ shared
#include "MixShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

#ifndef OSPRAY_TARGET_SYCL

OSPRAY_BEGIN_ISPC_NAMESPACE

#define MULTIBSDF_SIZE 2
struct Mix_BSDF
{
  DEFINE_MULTIBSDF(MULTIBSDF_SIZE);
  const varying BSDF *uniform bsdf1;
  const varying BSDF *uniform bsdf2;
};

SYCL_EXTERNAL const varying BSDF *uniform Mix_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh)
{
  const Mix *uniform self = (const Mix *uniform)super;

  // Allocate memory and initialize material BSDF
  varying Mix_BSDF *uniform bsdf =
      (varying Mix_BSDF * uniform) ShadingContext_alloc(ctx, sizeof(Mix_BSDF));
  varying MultiBSDF *uniform mbsdf = (varying MultiBSDF * uniform) bsdf;
  MultiBSDF_Constructor(mbsdf, MULTIBSDF_SIZE);
  bsdf->super.bsdfType = BSDF_TYPE_MIX;
  bsdf->bsdf1 = NULL;
  bsdf->bsdf2 = NULL;

  float factor = self->factor * clamp(get1f(self->factorMap, dg, 1.f));

  if (self->mat1) {
    bsdf->bsdf1 =
        self->mat1->getBSDF(self->mat1, ctx, dg, ray, currentMedium, ffh);
    if (bsdf->bsdf1)
      MultiBSDF_add(mbsdf, 0, bsdf->bsdf1, 1.0f - factor, 1.0f - factor);
  }

  if (self->mat2) {
    bsdf->bsdf2 =
        self->mat2->getBSDF(self->mat2, ctx, dg, ray, currentMedium, ffh);
    if (bsdf->bsdf2)
      MultiBSDF_add(mbsdf, 1, bsdf->bsdf2, factor, factor);
  }

  return &bsdf->super;
}

SYCL_EXTERNAL vec3f Mix_getTransparency(const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh)
{
  const Mix *uniform self = (const Mix *uniform)super;

  vec3f t1 = make_vec3f(0.f);
  if (self->mat1)
    t1 = self->mat1->getTransparency(self->mat1, dg, ray, currentMedium, ffh);

  vec3f t2 = make_vec3f(0.f);
  if (self->mat2)
    t2 = self->mat2->getTransparency(self->mat2, dg, ray, currentMedium, ffh);

  float factor = self->factor * clamp(get1f(self->factorMap, dg, 1.f));
  return lerp(factor, t1, t2);
}

SYCL_EXTERNAL vec3f Mix_getEmission(const Material *uniform super,
    const varying DifferentialGeometry &dg,
    const uniform FeatureFlagsHandler &ffh)
{
  const Mix *uniform self = (const Mix *uniform)super;

  vec3f t1 = make_vec3f(0.f);
  if (self->mat1)
    t1 = self->mat1->getEmission(self->mat1, dg, ffh);

  vec3f t2 = make_vec3f(0.f);
  if (self->mat2)
    t2 = self->mat2->getEmission(self->mat2, dg, ffh);

  float factor = self->factor * clamp(get1f(self->factorMap, dg, 1.f));
  return lerp(factor, t1, t2);
}

inline BSDF_EvalRes BSDF_dispatch_eval(
    const varying BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  uniform FeatureFlagsHandler ffh;
  return BSDF_dispatch_eval(self, wo, wi, ffh);
}

inline BSDF_SampleRes BSDF_dispatch_sample(
    const varying BSDF *uniform self, const vec3f &wo, const vec2f &s, float ss)
{
  uniform FeatureFlagsHandler ffh;
  return BSDF_dispatch_sample(self, wo, s, ss, ffh);
}

SYCL_EXTERNAL BSDF_EvalRes Mix_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying Mix_BSDF *uniform self = (const varying Mix_BSDF *uniform)super;

  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, self->bsdf1, BSDF_dispatch_eval);
  MULTIBSDF_EVAL_CHILD(1, self->bsdf2, BSDF_dispatch_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

SYCL_EXTERNAL BSDF_SampleRes Mix_BSDF_sample(const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying Mix_BSDF *uniform self = (const varying Mix_BSDF *uniform)super;

  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, self->bsdf1, BSDF_dispatch_sample);
  MULTIBSDF_SAMPLE_CHILD(1, self->bsdf2, BSDF_dispatch_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, self->bsdf1, BSDF_dispatch_eval);
  MULTIBSDF_EVAL_CHILD(1, self->bsdf2, BSDF_dispatch_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Mix_getBSDF_addr()
{
  return (void *uniform)Mix_getBSDF;
}

export void *uniform Mix_getTransparency_addr()
{
  return (void *uniform)Mix_getTransparency;
}

export void *uniform Mix_getEmission_addr()
{
  return (void *uniform)Mix_getEmission;
}

OSPRAY_END_ISPC_NAMESPACE

#endif
