// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OBJ.ih"

#include "render/Material.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/Specular.ih"
#include "render/bsdfs/Transmission.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "OBJShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

#define MULTIBSDF_SIZE 3
struct OBJ_BSDF
{
  DEFINE_MULTIBSDF(MULTIBSDF_SIZE);
  BSDF lambert;
  BSDF transmission;
  Specular specular;
};

SYCL_EXTERNAL const varying BSDF *uniform OBJ_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &ffh)
{
  uniform const OBJ *uniform self = (uniform const OBJ *uniform)super;

  // Allocate memory and initialize material BSDF
  varying OBJ_BSDF *uniform bsdf =
      (varying OBJ_BSDF * uniform) ShadingContext_alloc(ctx, sizeof(OBJ_BSDF));
  varying MultiBSDF *uniform mbsdf = (varying MultiBSDF * uniform) bsdf;
  MultiBSDF_Constructor(mbsdf, MULTIBSDF_SIZE);
  bsdf->super.bsdfType = BSDF_TYPE_OBJ;

  // textures modify (mul) values, see http://paulbourke.net/dataformats/mtl/

  // Normal map
  varying linear3f *uniform shadingFrame = LinearSpace3f_create(
      ctx, makeShadingFrame_ff(dg, self->bumpMap, self->bumpRot, ffh));

  /*! cut-out opacity */
  float d = self->d * get1f_ff(self->dMap, dg, 1.f, ffh) * dg.color.w;

  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);

  // Diffuse component
  vec3f Kd = self->Kd;
  if (valid(self->KdMap) && (ffo & FFO_TEXTURE_IN_MATERIAL)) {
    vec4f Kd_from_map = get4f(self->KdMap, dg);
    Kd = Kd * make_vec3f(Kd_from_map);
    d *= Kd_from_map.w;
  }

  // Lambert shading
  Kd = Kd * d * make_vec3f(dg.color);
  if (reduce_max(Kd) > 0.0f) {
    Lambert_Constructor(&bsdf->lambert, shadingFrame, Kd);
    MultiBSDF_add(mbsdf, 0, &bsdf->lambert, 1.f, luminance(Kd));
  }

  // Transmission component
  vec3f T = self->Tf * d + make_vec3f(1.f - d);
  if (reduce_max(T) > 0.0f) {
    Transmission_Constructor(&bsdf->transmission, shadingFrame, T);
    MultiBSDF_add(mbsdf, 1, &bsdf->transmission, 1.f, luminance(T));
  }

  /*! specular component */
  float Ns = self->Ns * get1f_ff(self->NsMap, dg, 1.0f, ffh);
  vec3f Ks = d * self->Ks * get3f_ff(self->KsMap, dg, make_vec3f(1.f), ffh);
  if (reduce_max(Ks) > 0.0f) {
    Specular_Constructor(&bsdf->specular, shadingFrame, Ks, Ns);
    MultiBSDF_add(mbsdf,
        2,
        (varying BSDF * uniform) & bsdf->specular,
        1.f,
        luminance(Ks));
  }
  return &bsdf->super;
}

SYCL_EXTERNAL vec3f OBJ_getTransparency(const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &,
    const uniform FeatureFlagsHandler &ffh)
{
  uniform const OBJ *uniform self = (uniform const OBJ *uniform)super;
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);

  /*! cut-out opacity */
  float d = self->d * get1f_ff(self->dMap, dg, 1.f, ffh) * dg.color.w;
  if (hasAlpha(self->KdMap) && (ffo & FFO_TEXTURE_IN_MATERIAL)) {
    vec4f Kd_from_map = get4f(self->KdMap, dg);
    d *= Kd_from_map.w;
  }

  // Transmission component
  vec3f T = self->Tf * d + make_vec3f(1.f - d);
  return T;
}

SYCL_EXTERNAL BSDF_EvalRes OBJ_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying OBJ_BSDF *uniform self = (const varying OBJ_BSDF *uniform)super;

  MULTIBSDF_EVAL_BEGIN();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, Lambert_eval);
  // 1 - omitted, no evaluation needed for transmission BSDF
  MULTIBSDF_EVAL_CHILD(2, &self->specular, Specular_eval);
  MULTIBSDF_EVAL_END();
  return MULTIBSDF_EVAL_GET();
}

SYCL_EXTERNAL BSDF_SampleRes OBJ_BSDF_sample(const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying OBJ_BSDF *uniform self = (const varying OBJ_BSDF *uniform)super;

  MULTIBSDF_SAMPLE_BEGIN();
  MULTIBSDF_SAMPLE_CHILD(0, &self->lambert, Lambert_sample);
  MULTIBSDF_SAMPLE_CHILD(1, &self->transmission, Transmission_sample);
  MULTIBSDF_SAMPLE_CHILD(2, &self->specular, Specular_sample);
  MULTIBSDF_SAMPLE_EVAL();
  MULTIBSDF_EVAL_CHILD(0, &self->lambert, Lambert_eval);
  // 1 - omitted, no evaluation needed for transmission BSDF
  MULTIBSDF_EVAL_CHILD(2, &self->specular, Specular_eval);
  MULTIBSDF_SAMPLE_END();
  return MULTIBSDF_SAMPLE_GET();
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform OBJ_getBSDF_addr()
{
  return (void *uniform)OBJ_getBSDF;
}

export void *uniform OBJ_getTransparency_addr()
{
  return (void *uniform)OBJ_getTransparency;
}

OSPRAY_END_ISPC_NAMESPACE
