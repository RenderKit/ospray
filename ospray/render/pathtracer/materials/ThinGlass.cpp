// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ThinGlass.h"
#include "texture/Texture2D.h"
// ispc
#include "ThinGlass_ispc.h"

namespace ospray {
namespace pathtracer {

ThinGlass::ThinGlass()
{
  ispcEquivalent = ispc::PathTracer_ThinGlass_create();
}

std::string ThinGlass::toString() const
{
  return "ospray::pathtracer::ThinGlass";
}

void ThinGlass::commit()
{
  const float eta = getParam<float>("eta", 1.5f);
  const vec3f &attenuationColor =
      getParam<vec3f>("attenuationColor", vec3f(1.f));
  const float attenuationDistance = getParam<float>("attenuationDistance", 1.f);
  const float thickness = getParam<float>("thickness", 1.f);

  Texture2D *map_attenuationColor =
      (Texture2D *)getParamObject("map_attenuationColor");
  affine2f xform_attenuationColor = getTextureTransform("map_attenuationColor");

  ispc::PathTracer_ThinGlass_set(getIE(),
      eta,
      (const ispc::vec3f &)attenuationColor,
      map_attenuationColor ? map_attenuationColor->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_attenuationColor,
      attenuationDistance,
      thickness);
}

} // namespace pathtracer
} // namespace ospray
