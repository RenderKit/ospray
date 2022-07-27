// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "lights/AmbientLight_ispc.h"
// ispc shared
#include "AmbientLightShared.h"

namespace ospray {

ispc::Light *AmbientLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ispc::AmbientLight *sh = StructSharedCreate<ispc::AmbientLight>();
  sh->super.sample = ispc::AmbientLight_sample_addr();
  sh->super.eval = ispc::AmbientLight_eval_addr();
  sh->super.isVisible = visible;
  sh->super.instance = instance;
  sh->radiance = radiance;
  return &sh->super;
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
    postStatusMsg(OSP_LOG_WARNING)
        << toString() << " unsupported 'intensityQuantity' value";
    radiance = vec3f(0.0f);
  }
}
} // namespace ospray
