// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "common/StructShared.h"
#ifndef OSPRAY_TARGET_SYCL
#include "lights/AmbientLight_ispc.h"
#else
namespace ispc {
void *AmbientLight_sample_addr();
void *AmbientLight_eval_addr();
} // namespace ispc
#endif
// ispc shared
#include "AmbientLightShared.h"

namespace ospray {

ISPCRTMemoryView AmbientLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ISPCRTMemoryView view = StructSharedCreate<ispc::AmbientLight>(
      getISPCDevice().getIspcrtContext().handle());
  ispc::AmbientLight *sh = (ispc::AmbientLight *)ispcrtSharedPtr(view);
#ifndef OSPRAY_TARGET_SYCL
  sh->super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
      ispc::AmbientLight_sample_addr());
  sh->super.eval =
      reinterpret_cast<ispc::Light_EvalFunc>(ispc::AmbientLight_eval_addr());
#endif
  sh->super.isVisible = visible;
  sh->super.instance = instance;
  sh->radiance = radiance;
  return view;
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
