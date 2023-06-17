// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ThinGlass.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/ThinGlass_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

ThinGlass::ThinGlass(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_MATERIAL_THINGLASS)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF = reinterpret_cast<ispc::Material_GetBSDFFunc>(
      ispc::ThinGlass_getBSDF_addr());
  getSh()->super.getTransparency =
      reinterpret_cast<ispc::Material_GetTransparencyFunc>(
          ispc::ThinGlass_getTransparency_addr());
#endif
}

std::string ThinGlass::toString() const
{
  return "ospray::pathtracer::ThinGlass";
}

void ThinGlass::commit()
{
  const float eta = getParam<float>("eta", 1.5f);
  MaterialParam3f attenuationColor =
      getMaterialParam3f("attenuationColor", vec3f(1.f));
  const float attenuationDistance = getParam<float>("attenuationDistance", 1.f);
  const float thickness = getParam<float>("thickness", 1.f);

  getSh()->eta = rcp(eta);
  getSh()->attenuationScale =
      thickness * rcp(std::max(attenuationDistance, EPS));
  const vec3f &acf = attenuationColor.factor;
  getSh()->attenuation =
      vec3f(log(acf.x), log(acf.y), log(acf.z)) * getSh()->attenuationScale;
  getSh()->attenuationColorMap = attenuationColor.tex;
}

} // namespace pathtracer
} // namespace ospray
