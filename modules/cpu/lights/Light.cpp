// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Light.h"
#ifndef OSPRAY_TARGET_SYCL
#include "lights/Light_ispc.h"
#endif

namespace ospray {

// Light definitions //////////////////////////////////////////////////////////

Light::Light(api::ISPCDevice &device, const FeatureFlagsOther ffo)
    : ISPCDeviceObject(device), featureFlags(ffo)
{
  managedObjectType = OSP_LIGHT;
}

void Light::commit()
{
  visible = getParam<bool>("visible", true);
  coloredIntensity =
      getParam<vec3f>("color", vec3f(1.f)) * getParam<float>("intensity", 1.f);
}

std::string Light::toString() const
{
  return "ospray::Light";
}

void Light::queryIntensityQuantityType(const OSPIntensityQuantity &defaultIQ)
{
  intensityQuantity =
      (OSPIntensityQuantity)getParam<uint8_t>("intensityQuantity", defaultIQ);
}

OSPTYPEFOR_DEFINITION(Light *);

} // namespace ospray
