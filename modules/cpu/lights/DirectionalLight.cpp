// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DirectionalLight.h"
#include "math/sampling.h"
// embree
#include "embree3/rtcore.h"
// ispc exports
#include "lights/DirectionalLight_ispc.h"
// ispc shared
#include "DirectionalLightShared.h"
#include "common/InstanceShared.h"

namespace ispc {

void DirectionalLight::set(bool isVisible,
    const Instance *instance,
    const vec3f &direction,
    const vec3f &irradiance,
    float cosAngle)
{
  super.isVisible = isVisible;
  super.instance = instance;
  super.sample = ispc::DirectionalLight_sample_addr();
  super.eval = ispc::DirectionalLight_eval_addr();

  frame = rkcommon::math::frame(direction);
  this->irradiance = irradiance;
  this->cosAngle = cosAngle;
  pdf = cosAngle < COS_ANGLE_MAX ? ospray::uniformSampleConePDF(cosAngle) : inf;

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    if (instance->motionBlur) {
      super.sample = ispc::DirectionalLight_sample_instanced_addr();
      super.eval = ispc::DirectionalLight_eval_instanced_addr();
    } else {
      frame = instance->xfm.l * frame;
    }
  }
}
} // namespace ispc

namespace ospray {

ispc::Light *DirectionalLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ispc::DirectionalLight *sh = StructSharedCreate<ispc::DirectionalLight>();
  sh->set(visible, instance, direction, irradiance, cosAngle);
  return &sh->super;
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
    postStatusMsg(OSP_LOG_WARNING)
        << toString() << " unsupported 'intensityQuantity' value";
    irradiance = vec3f(0.0f);
  }
}
} // namespace ospray
