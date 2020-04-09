// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MetallicPaint.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"
// ispc
#include "MetallicPaint_ispc.h"

namespace ospray {
namespace pathtracer {

MetallicPaint::MetallicPaint()
{
  ispcEquivalent = ispc::PathTracer_MetallicPaint_create();
}

std::string MetallicPaint::toString() const
{
  return "ospray::pathtracer::MetallicPaint";
}

void MetallicPaint::commit()
{
  const vec3f &color = getParam<vec3f>("baseColor", vec3f(0.8f));
  Texture2D *map_color = (Texture2D *)getParamObject("map_baseColor");
  affine2f xform_color = getTextureTransform("map_baseColor");
  const float flakeAmount = getParam<float>("flakeAmount", 0.3f);
  const vec3f &flakeColor = getParam<vec3f>("flakeColor", vec3f(RGB_AL_COLOR));
  const float flakeSpread = getParam<float>("flakeSpread", 0.5f);
  const float eta = getParam<float>("eta", 1.5f);

  ispc::PathTracer_MetallicPaint_set(getIE(),
      (const ispc::vec3f &)color,
      map_color ? map_color->getIE() : nullptr,
      (const ispc::AffineSpace2f &)xform_color,
      flakeAmount,
      (const ispc::vec3f &)flakeColor,
      flakeSpread,
      eta);
}

} // namespace pathtracer
} // namespace ospray
