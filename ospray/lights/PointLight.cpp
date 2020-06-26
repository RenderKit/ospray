// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PointLight.h"
#include "lights/PointLight_ispc.h"

namespace ospray {
PointLight::PointLight()
{
  ispcEquivalent = ispc::PointLight_create();
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

  // for the point light color * intensity
  // does not parameterize radiance but radiant intensity
  vec3f radIntensity = radiance;

  // converting radiant intensity to radiance
  // for the case that the point light represents an area
  // light source with a sphere shape
  if (radius > 0.0f) {
    vec3f power = 4.0f * M_PI * radIntensity;
    float sphereArea = 4.0f * M_PI * radius * radius;
    radiance = power / (M_PI * sphereArea);
  }

  ispc::PointLight_set(getIE(),
      (ispc::vec3f &)position,
      (ispc::vec3f &)radiance,
      (ispc::vec3f &)radIntensity,
      radius);
}

} // namespace ospray
