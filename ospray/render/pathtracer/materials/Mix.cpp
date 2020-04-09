// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Mix.h"
#include "texture/Texture2D.h"
// ispc
#include "Mix_ispc.h"

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
  float factor = getParam<float>("factor", 0.5f);
  Texture2D *map_factor = (Texture2D *)getParamObject("map_factor", nullptr);
  affine2f xform_factor = getTextureTransform("map_factor");
  ospray::Material *mat1 =
      (ospray::Material *)getParamObject("material1", nullptr);
  ospray::Material *mat2 =
      (ospray::Material *)getParamObject("material2", nullptr);

  ispc::PathTracer_Mix_set(ispcEquivalent,
      factor,
      map_factor ? map_factor->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_factor,
      mat1 ? mat1->getIE() : nullptr,
      mat2 ? mat2->getIE() : nullptr);
}

} // namespace pathtracer
} // namespace ospray
