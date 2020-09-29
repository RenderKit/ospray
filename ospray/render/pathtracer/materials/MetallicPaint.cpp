// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MetallicPaint.h"
#include "math/spectrum.h"
// ispc
#include "render/pathtracer/materials/MetallicPaint_ispc.h"

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
  MaterialParam3f color = getMaterialParam3f("baseColor", vec3f(.8f));
  const float flakeAmount = getParam<float>("flakeAmount", 0.3f);
  const vec3f &flakeColor = getParam<vec3f>("flakeColor", vec3f(RGB_AL_COLOR));
  const float flakeSpread = getParam<float>("flakeSpread", 0.5f);
  const float eta = getParam<float>("eta", 1.5f);

  ispc::PathTracer_MetallicPaint_set(getIE(),
      (const ispc::vec3f &)color.factor,
      color.tex,
      flakeAmount,
      (const ispc::vec3f &)flakeColor,
      flakeSpread,
      eta);
}

} // namespace pathtracer
} // namespace ospray
