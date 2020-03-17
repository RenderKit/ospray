// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PointLight.h"
#include "PointLight_ispc.h"

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

  ispc::PointLight_set(getIE(), (ispc::vec3f &)position, radius);
}

} // namespace ospray
