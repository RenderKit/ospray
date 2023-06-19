// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/bsdfs/BSDF.ih"

#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/Dielectric.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/LambertTransmission.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MicrofacetDielectric.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
#include "render/bsdfs/MicrofacetSheenLayer.ih"
#include "render/bsdfs/Minneart.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/OrenNayar.ih"
#include "render/bsdfs/Reflection.ih"
#include "render/bsdfs/RobustDielectric.ih"
#include "render/bsdfs/RobustThinDielectric.ih"
#include "render/bsdfs/Scale.ih"
#include "render/bsdfs/Specular.ih"
#include "render/bsdfs/ThinDielectric.ih"
#include "render/bsdfs/ThinMicrofacetDielectric.ih"
#include "render/bsdfs/Transmission.ih"
#include "render/bsdfs/Velvety.ih"

#include "common/FeatureFlags.ih"
#include "common/FeatureFlagsEnum.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

// Material feature flags to BSDF mapping for SYCL specialization constants
#define MTB_CONDUCTOR                                                          \
  (FFO_MATERIAL_ALLOY | FFO_MATERIAL_CARPAINT | FFO_MATERIAL_METAL             \
      | FFO_MATERIAL_PRINCIPLED)
#define MTB_DIELECTRIC FFO_MATERIAL_PRINCIPLED
#define MTB_DIELECTRIC_LAYER                                                   \
  (FFO_MATERIAL_CARPAINT | FFO_MATERIAL_METALLICPAINT | FFO_MATERIAL_PLASTIC   \
      | FFO_MATERIAL_PRINCIPLED)
#define MTB_LAMBERT                                                            \
  (FFO_MATERIAL_CARPAINT | FFO_MATERIAL_METALLICPAINT | FFO_MATERIAL_OBJ       \
      | FFO_MATERIAL_PLASTIC | FFO_MATERIAL_PRINCIPLED)
#define MTB_LAMBERT_TRANSMISSION FFO_MATERIAL_PRINCIPLED
#define MTB_MICROFACET_CONDUCTOR                                               \
  (FFO_MATERIAL_ALLOY | FFO_MATERIAL_CARPAINT | FFO_MATERIAL_METAL             \
      | FFO_MATERIAL_METALLICPAINT | FFO_MATERIAL_PRINCIPLED)
#define MTB_MICROFACET_DIELECTRIC FFO_MATERIAL_PRINCIPLED
#define MTB_MICROFACET_DIELECTRIC_LAYER                                        \
  (FFO_MATERIAL_CARPAINT | FFO_MATERIAL_PLASTIC | FFO_MATERIAL_PRINCIPLED)
#define MTB_MICROFACET_SHEEN_LAYER FFO_MATERIAL_PRINCIPLED
#define MTB_MINNEART FFO_MATERIAL_VELVET
#define MTB_MULTI_BSDF                                                         \
  (FFO_MATERIAL_METALLICPAINT | FFO_MATERIAL_MIX | FFO_MATERIAL_OBJ            \
      | FFO_MATERIAL_PRINCIPLED | FFO_MATERIAL_VELVET)
#define MTB_OREN_NAYAR (FFO_MATERIAL_CARPAINT | FFO_MATERIAL_PRINCIPLED)
#define MTB_SPECULAR FFO_MATERIAL_OBJ
#define MTB_SCALE 0
#define MTB_REFLECTION 0
#define MTB_ROBUST_DIELECTRIC FFO_MATERIAL_GLASS
#define MTB_ROBUST_THIN_DIELECTRIC FFO_MATERIAL_THINGLASS
#define MTB_THIN_DIELECTRIC FFO_MATERIAL_PRINCIPLED
#define MTB_THIN_MICROFACET_DIELECTRIC FFO_MATERIAL_PRINCIPLED
#define MTB_TRANSMISSION                                                       \
  (FFO_MATERIAL_LUMINOUS | FFO_MATERIAL_OBJ | FFO_MATERIAL_PRINCIPLED)
#define MTB_VELVETY FFO_MATERIAL_VELVET

static BSDF_EvalRes BSDF_dispatch_eval_nomulti(const varying BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_DIELECTRIC_LAYER)
      && (ffo & MTB_DIELECTRIC_LAYER)) {
    return DielectricLayer_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
      && (ffo & MTB_MICROFACET_DIELECTRIC_LAYER)) {
    return MicrofacetDielectricLayer_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_SCALE) && (ffo & MTB_SCALE)) {
    return Scale_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_SHEEN_LAYER)
      && (ffo & MTB_MICROFACET_SHEEN_LAYER)) {
    return MicrofacetSheenLayer_eval(self, wo, wi, ffh);
  } else {
    return BSDF_dispatch_eval_base(self, wo, wi, ffh);
  }
}

static BSDF_SampleRes BSDF_dispatch_sample_nomulti(
    const varying BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_DIELECTRIC_LAYER)
      && (ffo & MTB_DIELECTRIC_LAYER)) {
    return DielectricLayer_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
      && (ffo & MTB_MICROFACET_DIELECTRIC_LAYER)) {
    return MicrofacetDielectricLayer_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_SCALE) && (ffo & MTB_SCALE)) {
    return Scale_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_SHEEN_LAYER)
      && (ffo & MTB_MICROFACET_SHEEN_LAYER)) {
    return MicrofacetSheenLayer_sample(self, wo, s, ss, ffh);
  } else {
    return BSDF_dispatch_sample_base(self, wo, s, ss, ffh);
  }
}

SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval(const varying BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_MULTI_BSDF) && (ffo & MTB_MULTI_BSDF)) {
    return MultiBSDF_eval(self, wo, wi, ffh);
  } else {
    return BSDF_dispatch_eval_nomulti(self, wo, wi, ffh);
  }
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample(
    const varying BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_MULTI_BSDF) && (ffo & MTB_MULTI_BSDF)) {
    return MultiBSDF_sample(self, wo, s, ss, ffh);
  } else {
    return BSDF_dispatch_sample_nomulti(self, wo, s, ss, ffh);
  }
}

SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval_base(
    const varying BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_CONDUCTOR) && (ffo & MTB_CONDUCTOR)) {
    return Conductor_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_DIELECTRIC)
      && (ffo & MTB_DIELECTRIC)) {
    return Dielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_LAMBERT) && (ffo & MTB_LAMBERT)) {
    return Lambert_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_LAMBERT_TRANSMISSION)
      && (ffo & MTB_LAMBERT_TRANSMISSION)) {
    return LambertTransmission_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
      && (ffo & MTB_MICROFACET_CONDUCTOR)) {
    return MicrofacetConductor_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC)
      && (ffo & MTB_MICROFACET_DIELECTRIC)) {
    return MicrofacetDielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MINNEART) && (ffo & MTB_MINNEART)) {
    return Minneart_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_SPECULAR) && (ffo & MTB_SPECULAR)) {
    return Specular_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_OREN_NAYAR)
      && (ffo & MTB_OREN_NAYAR)) {
    return OrenNayar_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_REFLECTION)
      && (ffo & MTB_REFLECTION)) {
    return Reflection_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_ROBUST_DIELECTRIC)
      && (ffo & MTB_ROBUST_DIELECTRIC)) {
    return RobustDielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_ROBUST_THIN_DIELECTRIC)
      && (ffo & MTB_ROBUST_THIN_DIELECTRIC)) {
    return RobustThinDielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_THIN_DIELECTRIC)
      && (ffo & MTB_THIN_DIELECTRIC)) {
    return ThinDielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_THIN_MICROFACET_DIELECTRIC)
      && (ffo & MTB_THIN_MICROFACET_DIELECTRIC)) {
    return ThinMicrofacetDielectric_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_TRANSMISSION)
      && (ffo & MTB_TRANSMISSION)) {
    return Transmission_eval(self, wo, wi, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_VELVETY) && (ffo & MTB_VELVETY)) {
    return Velvety_eval(self, wo, wi, ffh);
  } else if (self->bsdfType == BSDF_TYPE_REALLY_UNKNOWN) {
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return self->eval(self, wo, wi, ffh);
#endif
  }
  return make_BSDF_EvalRes_zero();
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample_base(
    const varying BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_CONDUCTOR) && (ffo & MTB_CONDUCTOR)) {
    return Conductor_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_DIELECTRIC)
      && (ffo & MTB_DIELECTRIC)) {
    return Dielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_LAMBERT) && (ffo & MTB_LAMBERT)) {
    return Lambert_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_LAMBERT_TRANSMISSION)
      && (ffo & MTB_LAMBERT_TRANSMISSION)) {
    return LambertTransmission_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
      && (ffo & MTB_MICROFACET_CONDUCTOR)) {
    return MicrofacetConductor_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MICROFACET_DIELECTRIC)
      && (ffo & MTB_MICROFACET_DIELECTRIC)) {
    return MicrofacetDielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_MINNEART) && (ffo & MTB_MINNEART)) {
    return Minneart_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_OREN_NAYAR)
      && (ffo & MTB_OREN_NAYAR)) {
    return OrenNayar_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_SPECULAR) && (ffo & MTB_SPECULAR)) {
    return Specular_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_REFLECTION)
      && (ffo & MTB_REFLECTION)) {
    return Reflection_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_ROBUST_DIELECTRIC)
      && (ffo & MTB_ROBUST_DIELECTRIC)) {
    return RobustDielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_ROBUST_THIN_DIELECTRIC)
      && (ffo & MTB_ROBUST_THIN_DIELECTRIC)) {
    return RobustThinDielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_THIN_DIELECTRIC)
      && (ffo & MTB_THIN_DIELECTRIC)) {
    return ThinDielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_THIN_MICROFACET_DIELECTRIC)
      && (ffo & MTB_THIN_MICROFACET_DIELECTRIC)) {
    return ThinMicrofacetDielectric_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_TRANSMISSION)
      && (ffo & MTB_TRANSMISSION)) {
    return Transmission_sample(self, wo, s, ss, ffh);
  } else if ((self->bsdfType == BSDF_TYPE_VELVETY) && (ffo & MTB_VELVETY)) {
    return Velvety_sample(self, wo, s, ss, ffh);
  } else if (self->bsdfType == BSDF_TYPE_REALLY_UNKNOWN) {
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return self->sample(self, wo, s, ss, ffh);
#endif
  }
  return make_BSDF_SampleRes_zero();
}

OSPRAY_END_ISPC_NAMESPACE
