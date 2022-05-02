// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CylinderLight.h"
// embree
#include "embree3/rtcore.h"

#include "lights/CylinderLight_ispc.h"

#include "CylinderLightShared.h"
#include "common/InstanceShared.h"

namespace ospray {

ispc::Light *CylinderLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ispc::CylinderLight *sh = StructSharedCreate<ispc::CylinderLight>();
  sh->super.sample = ispc::CylinderLight_sample_addr();
  sh->super.eval = ispc::CylinderLight_eval_addr();
  sh->super.isVisible = visible;
  sh->super.instance = instance;

  const float zMax = length(position1 - position0);
  if (zMax <= 0.f || radius <= 0.f) {
    sh->radiance = 0.f;
    return &sh->super;
  }

  sh->radiance = radiance;
  sh->radius = radius;
  sh->pre.position0 = position0;
  sh->pre.position1 = position1;

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    if (instance->motionBlur) {
      sh->super.sample = ispc::CylinderLight_sample_instanced_addr();
      sh->super.eval = ispc::CylinderLight_eval_instanced_addr();
    } else
      ispc::CylinderLight_Transform(sh, instance->xfm, &sh->pre);
  }

  return &sh->super;
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
