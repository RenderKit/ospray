// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "lights/AmbientLight_ispc.h"

namespace ospray {

AmbientLight::AmbientLight()
{
  ispcEquivalent = ispc::AmbientLight_create();
}

std::string AmbientLight::toString() const
{
  return "ospray::AmbientLight";
}

void AmbientLight::commit()
{
  Light::commit();
  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();
  ispc::AmbientLight_set(getIE(), (ispc::vec3f &)radiance);
}

void AmbientLight::processIntensityQuantityType()
{
  // validate the correctness of the light quantity type
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_IRRADIANCE) {
    radiance = coloredIntensity / M_PI;
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    radiance = coloredIntensity;
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'ambient' light source");
    radiance = vec3f(0.0f);
  }
}
} // namespace ospray
