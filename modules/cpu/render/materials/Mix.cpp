// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Mix.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/Mix_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

MixMaterial::MixMaterial(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device, FFO_MATERIAL_MIX)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF =
      reinterpret_cast<ispc::Material_GetBSDFFunc>(ispc::Mix_getBSDF_addr());
  getSh()->super.getTransparency =
      reinterpret_cast<ispc::Material_GetTransparencyFunc>(
          ispc::Mix_getTransparency_addr());
  getSh()->super.getEmission = reinterpret_cast<ispc::Material_GetEmissionFunc>(
      ispc::Mix_getEmission_addr());
#endif
}

std::string MixMaterial::toString() const
{
  return "ospray::pathtracer::MixMaterial";
}

void MixMaterial::commit()
{
  factor = getMaterialParam1f("factor", .5f);
  mat1 = getParamObject<Material>("material1");
  mat2 = getParamObject<Material>("material2");

  getSh()->factor = clamp(factor.factor);
  getSh()->factorMap = factor.tex;
  getSh()->mat1 = mat1 ? mat1->getSh() : nullptr;
  getSh()->mat2 = mat2 ? mat2->getSh() : nullptr;
  getSh()->super.isEmissive = isEmissive();
}

} // namespace pathtracer
} // namespace ospray
