// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MetallicPaint.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/MetallicPaint_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

MetallicPaint::MetallicPaint(api::ISPCDevice &device)
    : AddStructShared(
        device.getIspcrtContext(), device, FFO_MATERIAL_METALLICPAINT)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF = reinterpret_cast<ispc::Material_GetBSDFFunc>(
      ispc::MetallicPaint_getBSDF_addr());
#endif
}

std::string MetallicPaint::toString() const
{
  return "ospray::pathtracer::MetallicPaint";
}

void MetallicPaint::commit()
{
  MaterialParam3f color = getMaterialParam3f("baseColor", vec3f(0.8f));
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
