// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Mix.h"
// ispc
#include "render/materials/Mix_ispc.h"

namespace ospray {
namespace pathtracer {

MixMaterial::MixMaterial()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_MIX;
  getSh()->super.getBSDF = ispc::Mix_getBSDF_addr();
  getSh()->super.getTransparency = ispc::Mix_getTransparency_addr();
}

std::string MixMaterial::toString() const
{
  return "ospray::pathtracer::MixMaterial";
}

void MixMaterial::commit()
{
  MaterialParam1f factor = getMaterialParam1f("factor", .5f);
  ospray::Material *mat1 =
      (ospray::Material *)getParamObject("material1", nullptr);
  ospray::Material *mat2 =
      (ospray::Material *)getParamObject("material2", nullptr);

  getSh()->factor = clamp(factor.factor);
  getSh()->factorMap = factor.tex;
  getSh()->mat1 = mat1 ? mat1->getSh() : nullptr;
  getSh()->mat2 = mat2 ? mat2->getSh() : nullptr;

  vec3f emission = vec3f(0.f);
  if (mat1)
    emission = (1.f - getSh()->factor) * getSh()->mat1->emission;
  if (mat2)
    emission = emission + getSh()->factor * getSh()->mat2->emission;
  getSh()->super.emission = emission;
}

} // namespace pathtracer
} // namespace ospray
