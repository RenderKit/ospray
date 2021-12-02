// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DirectionalLight.h"
#include "lights/DirectionalLight_ispc.h"
#include "lights/Light_ispc.h"

namespace ospray {

void *DirectionalLight::createIE(const void *instance) const
{
  void *ie = ispc::DirectionalLight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::DirectionalLight_set(
      ie, (ispc::vec3f &)irradiance, (ispc::vec3f &)direction, cosAngle);
  return ie;
}

std::string DirectionalLight::toString() const
{
  return "ospray::DirectionalLight";
}

void DirectionalLight::commit()
{
  Light::commit();
  direction = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  angularDiameter = getParam<float>("angularDiameter", .0f);

  // the ispc::DirLight expects direction towards light source
  direction = -normalize(direction);
  angularDiameter = clamp(angularDiameter, 0.f, 180.f);
  cosAngle = std::cos(deg2rad(0.5f * angularDiameter));

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_IRRADIANCE);
  processIntensityQuantityType();
}

void DirectionalLight::processIntensityQuantityType()
{
  // validate the correctness of the light quantity type
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    // convert from radiance to irradiance
    float cosineCapIntegral = 2.0f * M_PI * (1.0f - cosAngle);
    irradiance = cosineCapIntegral * coloredIntensity;
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_IRRADIANCE) {
    irradiance = coloredIntensity;
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'distant' light source");
    irradiance = vec3f(0.0f);
  }
}
} // namespace ospray
