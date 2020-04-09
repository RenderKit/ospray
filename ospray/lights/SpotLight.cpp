// Copyright 2009-2020 Intel Corporation
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
  auto position = getParam<vec3f>("position", vec3f(0.f));
  auto direction = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  auto openingAngle = getParam<float>("openingAngle", 180.f);
  auto penumbraAngle = getParam<float>("penumbraAngle", 5.f);

  // per default perpendicular to direction
  vec3f c0 = abs(direction.x) < abs(direction.y)
      ? vec3f(0.0f, direction.z, direction.y)
      : vec3f(direction.z, 0.0f, direction.x);
  c0 = getParam<vec3f>("c0", c0);
  lid = getParamDataT<float, 2>("intensityDistribution");
  vec2i size;
  if (lid) {
    if (lid->numItems.z > 1)
      throw std::runtime_error(toString()
          + " must have (at most 2D) 'intensityDistribution' array using the first two dimensions.");
    size = vec2i(lid->numItems.x, lid->numItems.y);
    if (size.x < 2)
      throw std::runtime_error(toString()
          + " 'intensityDistribution' must have data for at least two gamma angles.");
    if (!lid->compact()) {
      postStatusMsg(OSP_LOG_WARNING)
          << toString()
          << " does currently not support strides for 'intensityDistribution', copying data.";

      const auto data = new Data(OSP_FLOAT, lid->numItems);
      data->copy(*lid, vec3ui(0));
      lid = &(data->as<float, 2>());
      data->refDec();
    }
  }

  // check ranges and pre-compute parameters
  openingAngle = clamp(openingAngle, 0.f, 360.f);
  penumbraAngle = clamp(penumbraAngle, 0.f, 0.5f * openingAngle);

  const float cosAngleMax = std::cos(deg2rad(0.5f * openingAngle));
  const float cosAngleMin =
      std::cos(deg2rad(0.5f * openingAngle - penumbraAngle));
  const float cosAngleScale = 1.0f / (cosAngleMin - cosAngleMax);

  linear3f frame;
  frame.vz = normalize(direction);
  frame.vy = normalize(cross(c0, frame.vz));
  frame.vx = cross(frame.vz, frame.vy);

  ispc::SpotLight_set(getIE(),
      (const ispc::vec3f &)position,
      (const ispc::LinearSpace3f &)frame,
      cosAngleMax,
      cosAngleScale,
      getParam<float>("radius", 0.f),
      (const ispc::vec2i &)size,
      lid ? lid->data() : nullptr);
}

} // namespace ospray
