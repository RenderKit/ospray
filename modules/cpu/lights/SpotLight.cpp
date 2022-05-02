// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SpotLight.h"
#include "math/sampling.h"
// embree
#include "embree3/rtcore.h"

#include "lights/SpotLight_ispc.h"

#include "SpotLightShared.h"
#include "common/InstanceShared.h"

namespace ospray {

ispc::Light *SpotLight::createSh(uint32_t, const ispc::Instance *instance) const
{
  ispc::SpotLight *sh = StructSharedCreate<ispc::SpotLight>();
  sh->super.sample = ispc::SpotLight_sample_addr();
  sh->super.eval = ispc::SpotLight_eval_addr();
  sh->super.isVisible = visible;
  sh->super.instance = instance;

  sh->pre.position = position;
  sh->pre.direction = normalize(direction);

  intensityDistribution.setSh(sh->intensityDistribution);

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    sh->pre.c0 = intensityDistribution.c0;
    if (instance->motionBlur) {
      sh->super.sample = ispc::SpotLight_sample_instanced_addr();
      sh->super.eval = ispc::SpotLight_eval_instanced_addr();
    } else
      ispc::SpotLight_Transform(&sh->pre, instance->xfm, &sh->pre);
  } else {
    sh->pre.c90 = normalize(cross(intensityDistribution.c0, sh->pre.direction));
    sh->pre.c0 = cross(sh->pre.direction, sh->pre.c90);
  }

  sh->radiance = radiance;
  sh->intensity = radIntensity;
  sh->cosAngleMax = cosAngleMax;
  sh->cosAngleScale = cosAngleScale;
  sh->radius = radius;
  sh->innerRadius = innerRadius;
  sh->areaPdf = uniformSampleRingPDF(radius, innerRadius);

  return &sh->super;
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
