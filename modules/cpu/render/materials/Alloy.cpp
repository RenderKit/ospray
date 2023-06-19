// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Alloy.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/Alloy_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

Alloy::Alloy(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_MATERIAL_ALLOY)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF =
      reinterpret_cast<ispc::Material_GetBSDFFunc>(ispc::Alloy_getBSDF_addr());
#endif
}

std::string Alloy::toString() const
{
  return "ospray::pathtracer::Alloy";
}

//! \brief commit the material's parameters
void Alloy::commit()
{
  MaterialParam3f color = getMaterialParam3f("color", vec3f(0.9f));
  MaterialParam3f edgeColor = getMaterialParam3f("edgeColor", vec3f(1.f));
  MaterialParam1f roughness = getMaterialParam1f("roughness", 0.1f);

  getSh()->color = color.factor;
  getSh()->colorMap = color.tex;
  getSh()->edgeColor = edgeColor.factor;
  getSh()->edgeColorMap = edgeColor.tex;
  getSh()->roughness = roughness.factor;
  getSh()->roughnessMap = roughness.tex;
}

} // namespace pathtracer
} // namespace ospray
