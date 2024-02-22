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
    : AddStructShared(device.getIspcrtContext(), device, FFO_MATERIAL_MIX)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF =
      reinterpret_cast<ispc::Material_GetBSDFFunc>(ispc::Mix_getBSDF_addr());
  getSh()->super.getTransparency =
      reinterpret_cast<ispc::Material_GetTransparencyFunc>(
          ispc::Mix_getTransparency_addr());
#endif
}

std::string MixMaterial::toString() const
{
  return "ospray::pathtracer::MixMaterial";
}

void MixMaterial::commit()
{
  MaterialParam1f factor = getMaterialParam1f("factor", .5f);
  mat1 = getParamObject<Material>("material1");
  mat2 = getParamObject<Material>("material2");

  getSh()->factor = clamp(factor.factor);
  getSh()->factorMap = factor.tex;
  getSh()->mat1 = mat1 ? mat1->getSh() : nullptr;
  getSh()->mat2 = mat2 ? mat2->getSh() : nullptr;

  // XXX mixing emission(Map) is not supported, below are merely some WAs
  vec3f emission = vec3f(0.f);
  ispc::TextureParam emissionMap;
  if (mat1) {
    emission = (1.f - getSh()->factor) * getSh()->mat1->emission;
    if (getSh()->mat1->emissionMap.ptr)
      emissionMap = getSh()->mat1->emissionMap;
  }
  if (mat2) {
    emission = emission + getSh()->factor * getSh()->mat2->emission;
    // if both materials have emission texture, material2 wins
    if (getSh()->mat2->emissionMap.ptr)
      emissionMap = getSh()->mat2->emissionMap;
  }
  getSh()->super.emission = emission;
  getSh()->super.emissionMap = emissionMap;
}

} // namespace pathtracer
} // namespace ospray
