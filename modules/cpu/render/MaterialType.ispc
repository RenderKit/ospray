// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"
#include "MaterialType.ih"

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
    const Medium &currentMedium)
{
  switch (self->type) {
  case MATERIAL_TYPE_ALLOY:
    return Alloy_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_CARPAINT:
    return CarPaint_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_GLASS:
    return Glass_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_LUMINOUS:
    return Luminous_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_METAL:
    return Metal_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_METALLICPAINT:
    return MetallicPaint_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_MIX:
    return Mix_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_OBJ:
    return OBJ_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_PLASTIC:
    return Plastic_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_PRINCIPLED:
    return Principled_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_THINGLASS:
    return ThinGlass_getBSDF(self, ctx, dg, ray, currentMedium);
  case MATERIAL_TYPE_VELVET:
    return Velvet_getBSDF(self, ctx, dg, ray, currentMedium);
  default:
#ifndef OSPRAY_TARGET_DPCPP
    return self->getBSDF(self, ctx, dg, ray, currentMedium);
#else
    return NULL;
#endif
  }
  return NULL;
}

SYCL_EXTERNAL __noinline vec3f Material_dispatch_getTransparency(
    const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  switch (self->type) {
  case MATERIAL_TYPE_GLASS:
    return Glass_getTransparency(self, dg, ray, currentMedium);
  case MATERIAL_TYPE_LUMINOUS:
    return Luminous_getTransparency(self, dg, ray, currentMedium);
  case MATERIAL_TYPE_MIX:
    return Mix_getTransparency(self, dg, ray, currentMedium);
  case MATERIAL_TYPE_OBJ:
    return OBJ_getTransparency(self, dg, ray, currentMedium);
  case MATERIAL_TYPE_PRINCIPLED:
    return Principled_getTransparency(self, dg, ray, currentMedium);
  case MATERIAL_TYPE_THINGLASS:
    return ThinGlass_getTransparency(self, dg, ray, currentMedium);
  default:
#ifndef OSPRAY_TARGET_DPCPP
    return self->getTransparency(self, dg, ray, currentMedium);
#else
    return Material_getTransparency(self, dg, ray, currentMedium);
#endif
  }
}

SYCL_EXTERNAL __noinline void Material_dispatch_selectNextMedium(
    const uniform Material *uniform self,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{
  switch (self->type) {
  case MATERIAL_TYPE_GLASS:
    Glass_selectNextMedium(self, dg, currentMedium);
    break;
  case MATERIAL_TYPE_PRINCIPLED:
    Principled_selectNextMedium(self, dg, currentMedium);
    break;
  default:
#ifndef OSPRAY_TARGET_DPCPP
    self->selectNextMedium(self, dg, currentMedium);
#else
    Material_selectNextMedium(self, dg, currentMedium);
#endif
    break;
  }
}

OSPRAY_END_ISPC_NAMESPACE

