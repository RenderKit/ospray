// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PointLight.h"
// embree
#include "embree3/rtcore.h"

#include "lights/PointLight_ispc.h"

#include "PointLightShared.h"
#include "common/InstanceShared.h"

namespace ospray {

ispc::Light *PointLight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ispc::PointLight *sh = StructSharedCreate<ispc::PointLight>();
  sh->super.sample = ispc::PointLight_sample_addr();
  sh->super.eval = ispc::PointLight_eval_addr();
  sh->super.isVisible = visible;
  sh->super.instance = instance;

  sh->radiance = radiance;
  sh->intensity = radIntensity;
  sh->radius = radius;
  sh->pre.position = position;
  sh->pre.direction = normalize(direction);
  intensityDistribution.setSh(sh->intensityDistribution);

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    sh->pre.c0 = intensityDistribution.c0;
    if (instance->motionBlur) {
      sh->super.sample = ispc::PointLight_sample_instanced_addr();
      sh->super.eval = ispc::PointLight_eval_instanced_addr();
    } else
      ispc::PointLight_Transform(sh, instance->xfm, &sh->pre);
  } else {
    sh->pre.c90 = normalize(cross(intensityDistribution.c0, sh->pre.direction));
    sh->pre.c0 = cross(sh->pre.direction, sh->pre.c90);
  }

  return &sh->super;
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

  // per default perpendicular to direction
  direction = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  intensityDistribution.c0 = std::abs(direction.x) < std::abs(direction.y)
      ? vec3f(0.0f, direction.z, direction.y)
      : vec3f(direction.z, 0.0f, direction.x);
  intensityDistribution.readParams(*this);

  queryIntensityQuantityType(intensityDistribution
          ? OSP_INTENSITY_QUANTITY_SCALE
          : OSP_INTENSITY_QUANTITY_INTENSITY);
  processIntensityQuantityType();
}

void PointLight::processIntensityQuantityType()
{
  radIntensity = 0.0f;
  radiance = 0.0f;
  const float sphereArea = 4.0f * M_PI * radius * radius;

  // converting from the chosen intensity quantity type to radiance
  // (for r > 0) or radiative intensity (r = 0)
  if (intensityDistribution
          ? intensityQuantity == OSP_INTENSITY_QUANTITY_SCALE
          : intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radIntensity = coloredIntensity;
    if (radius > 0.0f) {
      // the visible surface are of a sphere in one direction is equal
      // to the surface area of a disk oriented to this direction
      radiance = coloredIntensity / (sphereArea / 4.0f);
    }
    return;
  }
  if (!intensityDistribution) {
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
      radIntensity = coloredIntensity / (4.0f * M_PI);
      if (radius > 0.0f)
        radiance = coloredIntensity / (M_PI * sphereArea);
      return;
    }
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
      // a virtual point light has no surface area therefore
      // radIntensity stays zero
      if (radius > 0.0f)
        radiance = coloredIntensity;
      return;
    }
  }

  postStatusMsg(OSP_LOG_WARNING)
      << toString() << " unsupported 'intensityQuantity' value";
}

} // namespace ospray
