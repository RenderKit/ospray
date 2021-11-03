// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "lights/AmbientLight_ispc.h"
#include "lights/Light_ispc.h"

namespace ospray {

void *AmbientLight::createIE(const void *instance) const
{
  void *ie = ispc::AmbientLight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::AmbientLight_set(ie, (ispc::vec3f &)radiance);
  return ie;
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
