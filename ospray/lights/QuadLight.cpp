// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "QuadLight.h"
#include "lights/QuadLight_ispc.h"

namespace ospray {

QuadLight::QuadLight()
{
  ispcEquivalent = ispc::QuadLight_create();
}

std::string QuadLight::toString() const
{
  return "ospray::QuadLight";
}

void QuadLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  edge1 = getParam<vec3f>("edge1", vec3f(1.f, 0.f, 0.f));
  edge2 = getParam<vec3f>("edge2", vec3f(0.f, 1.f, 0.f));

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();

  ispc::QuadLight_set(getIE(),
      (ispc::vec3f &)radiance,
      (ispc::vec3f &)position,
      (ispc::vec3f &)edge1,
      (ispc::vec3f &)edge2);
}

void QuadLight::processIntensityQuantityType()
{
  float quadArea = length(cross(edge1, edge2));
  /// converting from the chosen intensity quantity type to radiance
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
    radiance = coloredIntensity / (M_PI * quadArea);
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radiance = coloredIntensity / quadArea;
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    radiance = coloredIntensity;
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'quad' light source");
    radiance = vec3f(0.0f);
  }
}

} // namespace ospray
