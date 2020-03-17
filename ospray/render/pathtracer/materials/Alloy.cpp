// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Alloy.h"
#include "common/Data.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"
// ispc
#include "Alloy_ispc.h"

namespace ospray {
namespace pathtracer {

Alloy::Alloy()
{
  ispcEquivalent = ispc::PathTracer_Alloy_create();
}

std::string Alloy::toString() const
{
  return "ospray::pathtracer::Alloy";
}

//! \brief commit the material's parameters
void Alloy::commit()
{
  const vec3f &color = getParam<vec3f>("color", vec3f(0.9f));
  Texture2D *map_color = (Texture2D *)getParamObject("map_color");
  affine2f xform_color = getTextureTransform("map_color");

  const vec3f &edgeColor = getParam<vec3f>("edgeColor", vec3f(1.f));
  Texture2D *map_edgeColor = (Texture2D *)getParamObject("map_edgeColor");
  affine2f xform_edgeColor = getTextureTransform("map_edgeColor");

  const float roughness = getParam<float>("roughness", 0.1f);
  Texture2D *map_roughness = (Texture2D *)getParamObject("map_roughness");
  affine2f xform_roughness = getTextureTransform("map_roughness");

  ispc::PathTracer_Alloy_set(getIE(),
      (const ispc::vec3f &)color,
      map_color ? map_color->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_color,
      (const ispc::vec3f &)edgeColor,
      map_edgeColor ? map_edgeColor->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_edgeColor,
      roughness,
      map_roughness ? map_roughness->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_roughness);
}

} // namespace pathtracer
} // namespace ospray
