// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Velvet.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/materials/Velvet_ispc.h"
#endif

namespace ospray {
namespace pathtracer {

Velvet::Velvet(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device, FFO_MATERIAL_VELVET)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.getBSDF =
      reinterpret_cast<ispc::Material_GetBSDFFunc>(ispc::Velvet_getBSDF_addr());
#endif
}

std::string Velvet::toString() const
{
  return "ospray::pathtracer::Velvet";
}

void Velvet::commit()
{
  vec3f reflectance = getParam<vec3f>("reflectance", vec3f(.4f, 0.f, 0.f));
  float backScattering = getParam<float>("backScattering", .5f);
  vec3f horizonScatteringColor =
      getParam<vec3f>("horizonScatteringColor", vec3f(.75f, .1f, .1f));
  float horizonScatteringFallOff =
      getParam<float>("horizonScatteringFallOff", 10);

  getSh()->reflectance = reflectance;
  getSh()->backScattering = backScattering;
  getSh()->horizonScatteringColor = horizonScatteringColor;
  getSh()->horizonScatteringFallOff = horizonScatteringFallOff;
}

} // namespace pathtracer
} // namespace ospray
