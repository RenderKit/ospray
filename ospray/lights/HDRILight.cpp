// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HDRILight.h"
#include "lights/HDRILight_ispc.h"

namespace ospray {

HDRILight::HDRILight()
{
  ispcEquivalent = ispc::HDRILight_create();
}

HDRILight::~HDRILight()
{
  ispc::HDRILight_destroy(getIE());
  ispcEquivalent = nullptr;
}

std::string HDRILight::toString() const
{
  return "ospray::HDRILight";
}

void HDRILight::commit()
{
  Light::commit();
  up = getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f));
  dir = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  map = (Texture2D *)getParamObject("map", nullptr);

  linear3f frame;
  frame.vx = normalize(-dir);
  frame.vy = normalize(cross(frame.vx, up));
  frame.vz = cross(frame.vx, frame.vy);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();

  ispc::HDRILight_set(getIE(),
      (ispc::vec3f &)radianceScale,
      (const ispc::LinearSpace3f &)frame,
      map ? map->getIE() : nullptr);
}

void HDRILight::processIntensityQuantityType()
{
  radianceScale = coloredIntensity;
  // validate the correctness of the light quantity type
  if (intensityQuantity != OSP_INTENSITY_QUANTITY_RADIANCE) {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'hdri' light source");
    radianceScale = vec3f(0.0f);
  }
}

} // namespace ospray
