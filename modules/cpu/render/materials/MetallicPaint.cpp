// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MetallicPaint.h"
// ispc
#include "render/materials/MetallicPaint_ispc.h"

namespace ospray {
namespace pathtracer {

MetallicPaint::MetallicPaint()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_METALLICPAINT;
  getSh()->super.getBSDF = ispc::MetallicPaint_getBSDF_addr();
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

  getSh()->baseColor = color.factor * (1.f - flakeAmount);
  getSh()->baseColorMap = color.tex;
  getSh()->flakeAmount = flakeAmount;
  getSh()->flakeColor = flakeColor * flakeAmount;
  getSh()->flakeSpread = flakeSpread;
  getSh()->eta = rcp(eta);
}

} // namespace pathtracer
} // namespace ospray
