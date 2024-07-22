// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/bsdfs/BSDF.ih"

#include "common/FeatureFlags.ih"
#include "render/materials/Alloy.ih"
#include "render/materials/CarPaint.ih"
#include "render/materials/Glass.ih"
#include "render/materials/Luminous.ih"
#include "render/materials/Metal.ih"
#include "render/materials/MetallicPaint.ih"
#include "render/materials/Mix.ih"
#include "render/materials/OBJ.ih"
#include "render/materials/Plastic.ih"
#include "render/materials/Principled.ih"
#include "render/materials/ThinGlass.ih"
#include "render/materials/Velvet.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL BSDF_EvalRes BSDF_dispatch_eval(const varying BSDF *uniform self,
    const vec3f &wo,
    const vec3f &wi,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_OBJ) && (ffo & FFO_MATERIAL_OBJ)) {
    return OBJ_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_ALLOY_ROUGH)
      && (ffo & FFO_MATERIAL_ALLOY)) {
    return AlloyRough_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_METAL_ROUGH)
      && (ffo & FFO_MATERIAL_METAL)) {
    return MetalRough_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_METALLICPAINT)
      && (ffo & FFO_MATERIAL_METALLICPAINT)) {
    return MetallicPaint_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_CARPAINT)
      && (ffo & FFO_MATERIAL_CARPAINT)) {
    return CarPaint_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_PRINCIPLED)
      && (ffo & FFO_MATERIAL_PRINCIPLED)) {
    return Principled_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_PLASTIC)
      && (ffo & FFO_MATERIAL_PLASTIC)) {
    return Plastic_BSDF_eval(self, wo, wi);
  } else if ((self->bsdfType == BSDF_TYPE_VELVET)
      && (ffo & FFO_MATERIAL_VELVET)) {
    return Velvet_BSDF_eval(self, wo, wi);
  }
#ifndef OSPRAY_TARGET_SYCL
  else if ((self->bsdfType == BSDF_TYPE_MIX) && (ffo & FFO_MATERIAL_MIX)) {
    return Mix_BSDF_eval(self, wo, wi);
  }
#endif

  return make_BSDF_EvalRes_zero();
}

SYCL_EXTERNAL BSDF_SampleRes BSDF_dispatch_sample(
    const varying BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->bsdfType == BSDF_TYPE_OBJ) && (ffo & FFO_MATERIAL_OBJ)) {
    return OBJ_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_THINGLASS)
      && (ffo & FFO_MATERIAL_THINGLASS)) {
    return ThinGlass_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_GLASS)
      && (ffo & FFO_MATERIAL_GLASS)) {
    return Glass_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_LUMINOUS)
      && (ffo & FFO_MATERIAL_LUMINOUS)) {
    return Luminous_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_ALLOY)
      && (ffo & FFO_MATERIAL_ALLOY)) {
    return Alloy_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_ALLOY_ROUGH)
      && (ffo & FFO_MATERIAL_ALLOY)) {
    return AlloyRough_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_METAL)
      && (ffo & FFO_MATERIAL_METAL)) {
    return Metal_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_METAL_ROUGH)
      && (ffo & FFO_MATERIAL_METAL)) {
    return MetalRough_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_METALLICPAINT)
      && (ffo & FFO_MATERIAL_METALLICPAINT)) {
    return MetallicPaint_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_CARPAINT)
      && (ffo & FFO_MATERIAL_CARPAINT)) {
    return CarPaint_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_PRINCIPLED)
      && (ffo & FFO_MATERIAL_PRINCIPLED)) {
    return Principled_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_PLASTIC)
      && (ffo & FFO_MATERIAL_PLASTIC)) {
    return Plastic_BSDF_sample(self, wo, s, ss);
  } else if ((self->bsdfType == BSDF_TYPE_VELVET)
      && (ffo & FFO_MATERIAL_VELVET)) {
    return Velvet_BSDF_sample(self, wo, s, ss);
  }
#ifndef OSPRAY_TARGET_SYCL
  else if ((self->bsdfType == BSDF_TYPE_MIX) && (ffo & FFO_MATERIAL_MIX)) {
    return Mix_BSDF_sample(self, wo, s, ss);
  }
#endif

  return make_BSDF_SampleRes_zero();
}

OSPRAY_END_ISPC_NAMESPACE
