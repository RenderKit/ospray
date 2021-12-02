// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CylinderLight.h"
#include "lights/CylinderLight_ispc.h"
#include "lights/Light_ispc.h"

namespace ospray {

CylinderLight::CylinderLight()
{
  ispcEquivalent = ispc::CylinderLight_create();
}

void *CylinderLight::createIE(const void *instance) const
{
  void *ie = ispc::CylinderLight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::CylinderLight_set(ie,
      (ispc::vec3f &)radiance,
      (ispc::vec3f &)position0,
      (ispc::vec3f &)position1,
      radius);
  return ie;
}

std::string CylinderLight::toString() const
{
  return "ospray::CylinderLight";
}

void CylinderLight::commit()
{
  Light::commit();
  position0 = getParam<vec3f>("position0", vec3f(0.f));
  position1 = getParam<vec3f>("position1", vec3f(0.f, 0.f, 1.f));
  radius = getParam<float>("radius", 1.f);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();
}

void CylinderLight::processIntensityQuantityType()
{
  float cylinderArea = 2.f * M_PI * length(position1 - position0) * radius;
  radiance = vec3f(0.0f);
  /// converting from the chosen intensity quantity type to radiance
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
    if (cylinderArea > 0.f)
      radiance = coloredIntensity / (M_PI * cylinderArea);
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    if (cylinderArea > 0.f)
      radiance = M_PI * coloredIntensity / cylinderArea;
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    radiance = coloredIntensity;
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'cylinder' light source");
  }
}

} // namespace ospray
