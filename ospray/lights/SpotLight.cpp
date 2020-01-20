// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SpotLight.h"
#include "SpotLight_ispc.h"

namespace ospray {

SpotLight::SpotLight()
{
  ispcEquivalent = ispc::SpotLight_create();
}

std::string SpotLight::toString() const
{
  return "ospray::SpotLight";
}

void SpotLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  direction = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  openingAngle = getParam<float>("openingAngle", 180.f);
  penumbraAngle = getParam<float>("penumbraAngle", 5.f);
  radius = getParam<float>("radius", 0.f);

  // check ranges and pre-compute parameters
  direction = normalize(direction);
  openingAngle = clamp(openingAngle, 0.f, 180.f);
  penumbraAngle = clamp(penumbraAngle, 0.f, 0.5f * openingAngle);

  const float cosAngleMax = std::cos(deg2rad(0.5f * openingAngle));
  const float cosAngleMin =
      std::cos(deg2rad(0.5f * openingAngle - penumbraAngle));
  const float cosAngleScale = 1.0f / (cosAngleMin - cosAngleMax);

  ispc::SpotLight_set(getIE(),
      (ispc::vec3f &)position,
      (ispc::vec3f &)direction,
      cosAngleMax,
      cosAngleScale,
      radius);
}

OSP_REGISTER_LIGHT(SpotLight, spot);

} // namespace ospray
