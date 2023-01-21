// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Plastic.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/Plastic_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

Plastic::Plastic(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_MATERIAL_PLASTIC)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF = reinterpret_cast<ispc::Material_GetBSDFFunc>(
      ispc::Plastic_getBSDF_addr());
#endif
}

std::string Plastic::toString() const
{
  return "ospray::pathtracer::Plastic";
}

void Plastic::commit()
{
  const vec3f pigmentColor = getParam<vec3f>("pigmentColor", vec3f(1.f));
  const float eta = getParam<float>("eta", 1.4f);
  const float roughness = getParam<float>("roughness", 0.01f);

  getSh()->pigmentColor = pigmentColor;
  getSh()->eta = rcp(eta);
  getSh()->roughness = roughness;
}

} // namespace pathtracer
} // namespace ospray
