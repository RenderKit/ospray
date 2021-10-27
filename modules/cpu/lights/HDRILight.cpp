// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HDRILight.h"
#include "lights/HDRILight_ispc.h"
#include "lights/Light_ispc.h"

namespace ospray {

HDRILight::~HDRILight()
{
  ispc::HDRILight_destroyDistribution(distributionIE);
}

void *HDRILight::createIE(const void *instance) const
{
  void *ie = ispc::HDRILight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::HDRILight_set(ie,
      (ispc::vec3f &)coloredIntensity,
      (const ispc::LinearSpace3f &)frame,
      map ? map->getSh() : nullptr,
      distributionIE);
  return ie;
}

std::string HDRILight::toString() const
{
  return "ospray::HDRILight";
}

void HDRILight::commit()
{
  Light::commit();
  const vec3f up = getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f));
  const vec3f dir = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  map = (Texture2D *)getParamObject("map", nullptr);

  // recreate distribution
  ispc::HDRILight_destroyDistribution(distributionIE);
  distributionIE = nullptr;
  if (map)
    distributionIE = ispc::HDRILight_createDistribution(map->getSh());

  frame.vx = normalize(-dir);
  frame.vy = normalize(cross(frame.vx, up));
  frame.vz = cross(frame.vx, frame.vy);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_SCALE);
  processIntensityQuantityType();
}

void HDRILight::processIntensityQuantityType()
{
  // validate the correctness of the light quantity type
  if (intensityQuantity != OSP_INTENSITY_QUANTITY_SCALE
      && intensityQuantity != OSP_INTENSITY_QUANTITY_RADIANCE) {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'hdri' light source");
    coloredIntensity = vec3f(0.0f);
  }
}

} // namespace ospray
