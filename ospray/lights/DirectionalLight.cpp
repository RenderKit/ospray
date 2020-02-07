// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DirectionalLight.h"
#include "DirectionalLight_ispc.h"

namespace ospray {

DirectionalLight::DirectionalLight()
{
  ispcEquivalent = ispc::DirectionalLight_create();
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
  const float cosAngle = std::cos(deg2rad(0.5f * angularDiameter));

  ispc::DirectionalLight_set(getIE(), (ispc::vec3f &)direction, cosAngle);
}

OSP_REGISTER_LIGHT(DirectionalLight, distant);

} // namespace ospray
