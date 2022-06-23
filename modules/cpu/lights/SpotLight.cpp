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
      (const ispc::vec3f &)intensityDistribution.c0,
      (const ispc::vec3f &)radiance,
      (const ispc::vec3f &)radIntensity,
      cosAngleMax,
      cosAngleScale,
      radius,
      innerRadius,
      (const ispc::vec2i &)intensityDistribution.size,
      intensityDistribution.data());
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
  intensityDistribution.c0 = std::abs(direction.x) < std::abs(direction.y)
      ? vec3f(0.0f, direction.z, direction.y)
      : vec3f(direction.z, 0.0f, direction.x);
  intensityDistribution.readParams(*this);

  // check ranges and pre-compute parameters
  openingAngle = clamp(openingAngle, 0.f, 360.f);
  penumbraAngle = clamp(penumbraAngle, 0.f, 0.5f * openingAngle);

  cosAngleMax = std::cos(deg2rad(0.5f * openingAngle));
  const float cosAngleMin =
      std::cos(deg2rad(0.5f * openingAngle - penumbraAngle));
  cosAngleScale = 1.0f / (cosAngleMin - cosAngleMax);

  queryIntensityQuantityType(intensityDistribution
          ? OSP_INTENSITY_QUANTITY_SCALE
          : OSP_INTENSITY_QUANTITY_INTENSITY);
  processIntensityQuantityType(openingAngle);
}

void SpotLight::processIntensityQuantityType(const float openingAngle)
{
  radIntensity = 0.0f;
  radiance = 0.0f;
  const float halfOpeningAngleRad = M_PI * (openingAngle * 0.5f) / 180.0f;
  const float cosHalfOpeningAngle = cos(halfOpeningAngleRad);
  const auto sqr = [](const float f) { return f * f; };
  const float ringDiskArea = M_PI * (sqr(radius) - sqr(innerRadius));
  const float sphericalCapCosInt = M_PI * (1.0f - sqr(cosHalfOpeningAngle));

  // converting from the chosen intensity quantity type to radiance
  if (intensityDistribution
          ? intensityQuantity == OSP_INTENSITY_QUANTITY_SCALE
          : intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radIntensity = coloredIntensity;
    if (radius > 0.0f)
      radiance = radIntensity / ringDiskArea;
    return;
  }
  if (!intensityDistribution) {
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
      // since our spot light implementation includes the cosine term we need
      // to consider the integrated cosine cap instead of the usually used
      // integrated cap
      radIntensity = coloredIntensity / sphericalCapCosInt;
      if (radius > 0.0f)
        radiance = coloredIntensity / (sphericalCapCosInt * ringDiskArea);
      return;
    }
  }
  // XXX support for RADIANCE with intensityDistribution is deprecated
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    // a virtual spot light has no surface area therefore radIntensity stays 0
    if (radius > 0.0f)
      radiance = coloredIntensity;
    return;
  }

  postStatusMsg(OSP_LOG_WARNING)
      << toString() << " unsupported 'intensityQuantity' value";
}

} // namespace ospray
