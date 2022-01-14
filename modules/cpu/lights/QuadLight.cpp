// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "QuadLight.h"
#include "lights/Light_ispc.h"
#include "lights/QuadLight_ispc.h"

namespace ospray {

void *QuadLight::createIE(const void *instance) const
{
  void *ie = ispc::QuadLight_create();
  ispc::Light_set(ie, visible, (const ispc::Instance *)instance);
  ispc::QuadLight_set(ie,
      (ispc::vec3f &)radiance,
      (ispc::vec3f &)position,
      (ispc::vec3f &)edge1,
      (ispc::vec3f &)edge2,
      intensityDistribution.data(),
      (const ispc::vec2i &)intensityDistribution.size,
      (const ispc::vec3f &)intensityDistribution.c0);
  return ie;
}

std::string QuadLight::toString() const
{
  return "ospray::QuadLight";
}

void QuadLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  edge1 = getParam<vec3f>("edge1", vec3f(1.f, 0.f, 0.f));
  edge2 = getParam<vec3f>("edge2", vec3f(0.f, 1.f, 0.f));

  intensityDistribution.c0 = edge2;
  intensityDistribution.readParams(*this);

  queryIntensityQuantityType(intensityDistribution
          ? OSP_INTENSITY_QUANTITY_SCALE
          : OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();
}

void QuadLight::processIntensityQuantityType()
{
  const float quadArea = length(cross(edge1, edge2));

  // converting from the chosen intensity quantity type to radiance
  if (intensityDistribution
          ? intensityQuantity == OSP_INTENSITY_QUANTITY_SCALE
          : intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radiance = coloredIntensity / quadArea;
    return;
  }
  if (!intensityDistribution) {
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
      radiance = coloredIntensity / (M_PI * quadArea);
      return;
    }
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
      radiance = coloredIntensity;
      return;
    }
  }
  postStatusMsg(OSP_LOG_WARNING)
      << toString() << " unsupported 'intensityQuantity' value";
  radiance = vec3f(0.0f);
}

} // namespace ospray
