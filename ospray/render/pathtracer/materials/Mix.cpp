// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Mix.h"
// ispc
#include "render/pathtracer/materials/Mix_ispc.h"

namespace ospray {
namespace pathtracer {

MixMaterial::MixMaterial()
{
  ispcEquivalent = ispc::PathTracer_Mix_create();
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

  ispc::PathTracer_Mix_set(ispcEquivalent,
      factor.factor,
      factor.tex,
      mat1 ? mat1->getIE() : nullptr,
      mat2 ? mat2->getIE() : nullptr);
}

} // namespace pathtracer
} // namespace ospray
