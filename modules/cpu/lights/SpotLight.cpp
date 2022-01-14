// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SpotLight.h"
#include "lights/Light_ispc.h"
#include "lights/SpotLight_ispc.h"

namespace ospray {

void *SpotLight::createIE(const void *instance) const
{
  void *ie = ispc::SpotLight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::SpotLight_set(ie,
      (const ispc::vec3f &)position,
      (const ispc::vec3f &)direction,
      (const ispc::vec3f &)c0,
      (const ispc::vec3f &)radiance,
      (const ispc::vec3f &)radIntensity,
      cosAngleMax,
      cosAngleScale,
      radius,
      innerRadius,
      (const ispc::vec2i &)size,
      lid ? lid->data() : nullptr);
  return ie;
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
  float openingAngle = getParam<float>("openingAngle", 180.f);
  float penumbraAngle = getParam<float>("penumbraAngle", 5.f);
  radius = max(0.0f, getParam<float>("radius", 0.f));
  innerRadius =
      clamp(getParam<float>("innerRadius", 0.f), 0.0f, 0.999f * radius);

  // per default perpendicular to direction
  c0 = std::abs(direction.x) < std::abs(direction.y)
      ? vec3f(0.0f, direction.z, direction.y)
      : vec3f(direction.z, 0.0f, direction.x);
  c0 = getParam<vec3f>("c0", c0);
  lid = getParamDataT<float, 2>("intensityDistribution");
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

  cosAngleMax = std::cos(deg2rad(0.5f * openingAngle));
  const float cosAngleMin =
      std::cos(deg2rad(0.5f * openingAngle - penumbraAngle));
  cosAngleScale = 1.0f / (cosAngleMin - cosAngleMax);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_INTENSITY);
  processIntensityQuantityType(openingAngle);
}

void SpotLight::processIntensityQuantityType(const float openingAngle)
{
  radIntensity = 0.0f;
  radiance = 0.0f;

  float halfOpeningAngleRad = M_PI * (openingAngle * 0.5f) / 180.0f;
  float cosHalfOpeningAngle = cos(halfOpeningAngleRad);
  float ringDiskArea = M_PI * (radius * radius - innerRadius * innerRadius);

  float sphericalCapCosInt =
      M_PI * (1.0f - cosHalfOpeningAngle * cosHalfOpeningAngle);

  if (intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radIntensity = coloredIntensity;
    if (radius > 0.0f) {
      radiance = radIntensity / ringDiskArea;
    }
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
    // since our spot light implementation includes the cosine term
    // we need to consider the integrated cosine cap instead of the
    // usually used integrated cap.
    radIntensity = coloredIntensity / sphericalCapCosInt;
    if (radius > 0.0f) {
      radiance = coloredIntensity / (sphericalCapCosInt * ringDiskArea);
    }
    if (lid) {
      static WarnOnce warning(
          "The 'intensityQuantity' : 'OSP_INTENSITY_QUANTITY_POWER' is not supported when using an 'intensityDistribution'");
      radIntensity = 0.0f;
      radiance = 0.0f;
    }
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    // a virtual spot light has no surface area
    // therefore radIntensity stays zero and radiance is only
    // set if radius > 0
    if (radius > 0.0f) {
      radiance = coloredIntensity;
    }
  } else {
    static WarnOnce warning(
        "Unsupported intensityQuantity type for a 'spot' light source");
    radiance = vec3f(0.0f);
  }
}

} // namespace ospray
