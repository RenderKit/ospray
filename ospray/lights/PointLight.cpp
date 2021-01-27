// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PointLight.h"
#include "lights/PointLight_ispc.h"

namespace ospray {
PointLight::PointLight()
{
  ispcEquivalent = ispc::PointLight_create();
}

std::string PointLight::toString() const
{
  return "ospray::PointLight";
}

void PointLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  radius = getParam<float>("radius", 0.f);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_INTENSITY);
  vec3f radIntensity = 0.0f;
  processIntensityQuantityType(radIntensity);

  ispc::PointLight_set(getIE(),
      (ispc::vec3f &)position,
      (ispc::vec3f &)radiance,
      (ispc::vec3f &)radIntensity,
      radius);
}

void PointLight::processIntensityQuantityType(vec3f &radIntensity)
{
  radIntensity = 0.0f;
  radiance = 0.0f;
  float sphereArea = 0.0f;
  if (radius > 0.0f) {
    sphereArea = 4.0f * M_PI * radius * radius;
  }

  // converting from the chosen intensity quantity type to radiance
  // (for r > 0) or radiative intensity (r = 0)
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radIntensity = coloredIntensity;
    if (radius > 0.0f) {
      // the visible surface are of a sphere in one direction
      // is equal to the surface area of a disk oriented to this
      // direction
      radiance = coloredIntensity / (sphereArea / 4.0f);
    }
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
    radIntensity = coloredIntensity / (4.0f * M_PI);
    if (radius > 0.0f) {
      radiance = coloredIntensity / (M_PI * sphereArea);
    }
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    // a virtual point light has no surface area
    // therefore radIntensity stays zero and radiance is only
    // set if radius > 0
    if (radius > 0.0f) {
      radiance = coloredIntensity;
    }
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'point' light source");
    radiance = vec3f(0.0f);
  }
}

} // namespace ospray
