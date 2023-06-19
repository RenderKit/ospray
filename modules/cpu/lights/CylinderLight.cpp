// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CylinderLight.h"
#include "common/StructShared.h"
#ifndef OSPRAY_TARGET_SYCL
#include "lights/CylinderLight_ispc.h"
#else
namespace ispc {
void CylinderLight_Transform(const void *self, const void *xfm, void *dyn);
void *CylinderLight_sample_addr();
void *CylinderLight_sample_instanced_addr();
void *CylinderLight_eval_addr();
void *CylinderLight_eval_instanced_addr();
} // namespace ispc
#endif

#include "CylinderLightShared.h"
#include "common/InstanceShared.h"

namespace ospray {

ISPCRTMemoryView CylinderLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ISPCRTMemoryView view = StructSharedCreate<ispc::CylinderLight>(
      getISPCDevice().getIspcrtContext().handle());
  ispc::CylinderLight *sh = (ispc::CylinderLight *)ispcrtSharedPtr(view);

#ifndef OSPRAY_TARGET_SYCL
  sh->super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
      ispc::CylinderLight_sample_addr());
  sh->super.eval =
      reinterpret_cast<ispc::Light_EvalFunc>(ispc::CylinderLight_eval_addr());
#endif
  sh->super.isVisible = visible;
  sh->super.instance = instance;

  const float zMax = length(position1 - position0);
  if (zMax <= 0.f || radius <= 0.f) {
    sh->radiance = 0.f;
    return view;
  }

  sh->radiance = radiance;
  sh->radius = radius;
  sh->pre.position0 = position0;
  sh->pre.position1 = position1;

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    if (instance->motionBlur) {
#ifndef OSPRAY_TARGET_SYCL
      sh->super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
          ispc::CylinderLight_sample_instanced_addr());
      sh->super.eval = reinterpret_cast<ispc::Light_EvalFunc>(
          ispc::CylinderLight_eval_instanced_addr());
#endif
    } else
      ispc::CylinderLight_Transform(sh, instance->xfm, &sh->pre);
  }

  return view;
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
    postStatusMsg(OSP_LOG_WARNING)
        << toString() << " unsupported 'intensityQuantity' value";
  }
}

} // namespace ospray
