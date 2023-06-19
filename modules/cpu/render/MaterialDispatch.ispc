// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"
#include "MaterialDispatch.ih"

#include "common/FeatureFlagsEnum.h"
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

SYCL_EXTERNAL __noinline const varying BSDF *varying Material_dispatch_getBSDF(
    const uniform Material *uniform self,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->type == MATERIAL_TYPE_ALLOY) && (ffo & FFO_MATERIAL_ALLOY)) {
    return Alloy_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_CARPAINT)
      && (ffo & FFO_MATERIAL_CARPAINT)) {
    return CarPaint_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_GLASS)
      && (ffo & FFO_MATERIAL_GLASS)) {
    return Glass_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_LUMINOUS)
      && (ffo & FFO_MATERIAL_LUMINOUS)) {
    return Luminous_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_METAL)
      && (ffo & FFO_MATERIAL_METAL)) {
    return Metal_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_METALLICPAINT)
      && (ffo & FFO_MATERIAL_METALLICPAINT)) {
    return MetallicPaint_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_MIX) && (ffo & FFO_MATERIAL_MIX)) {
    return Mix_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_OBJ) && (ffo & FFO_MATERIAL_OBJ)) {
    return OBJ_getBSDF(self, ctx, dg, ray, currentMedium, ffh);
  } else if ((self->type == MATERIAL_TYPE_PLASTIC)
      && (ffo & FFO_MATERIAL_PLASTIC)) {
    return Plastic_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_PRINCIPLED)
      && (ffo & FFO_MATERIAL_PRINCIPLED)) {
    return Principled_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_THINGLASS)
      && (ffo & FFO_MATERIAL_THINGLASS)) {
    return ThinGlass_getBSDF(self, ctx, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_VELVET)
      && (ffo & FFO_MATERIAL_VELVET)) {
    return Velvet_getBSDF(self, ctx, dg, ray, currentMedium);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return self->getBSDF(self, ctx, dg, ray, currentMedium);
#endif
  }
  return NULL;
}

SYCL_EXTERNAL __noinline vec3f Material_dispatch_getTransparency(
    const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->type == MATERIAL_TYPE_GLASS) && (ffo & FFO_MATERIAL_GLASS)) {
    return Glass_getTransparency(self, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_LUMINOUS)
      && (ffo & FFO_MATERIAL_LUMINOUS)) {
    return Luminous_getTransparency(self, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_MIX) && (ffo & FFO_MATERIAL_MIX)) {
    return Mix_getTransparency(self, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_OBJ) && (ffo & FFO_MATERIAL_OBJ)) {
    return OBJ_getTransparency(self, dg, ray, currentMedium, ffh);
  } else if ((self->type == MATERIAL_TYPE_PRINCIPLED)
      && (ffo & FFO_MATERIAL_PRINCIPLED)) {
    return Principled_getTransparency(self, dg, ray, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_THINGLASS)
      && (ffo & FFO_MATERIAL_THINGLASS)) {
    return ThinGlass_getTransparency(self, dg, ray, currentMedium);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return self->getTransparency(self, dg, ray, currentMedium);
#else
    return Material_getTransparency(self, dg, ray, currentMedium);
#endif
  }
}

SYCL_EXTERNAL __noinline void Material_dispatch_selectNextMedium(
    const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    Medium &currentMedium,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->type == MATERIAL_TYPE_GLASS) && (ffo & FFO_MATERIAL_GLASS)) {
    Glass_selectNextMedium(self, dg, currentMedium);
  } else if ((self->type == MATERIAL_TYPE_PRINCIPLED)
      && (ffo & FFO_MATERIAL_PRINCIPLED)) {
    Principled_selectNextMedium(self, dg, currentMedium);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    self->selectNextMedium(self, dg, currentMedium);
#else
    Material_selectNextMedium(self, dg, currentMedium);
#endif
  }
}

OSPRAY_END_ISPC_NAMESPACE
