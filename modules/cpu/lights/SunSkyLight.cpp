// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SunSkyLight.h"
#include "texture/Texture2D.h"
// embree
#include "embree3/rtcore.h"
// ispc exports
#include "lights/HDRILight_ispc.h"
// ispc shared
#include "DirectionalLightShared.h"
#include "HDRILightShared.h"

namespace ospray {

SunSkyLight::SunSkyLight()
{
  static const int skyResolution = 512;
  skySize = vec2i(skyResolution, skyResolution / 2);
  skyImage.resize(skySize.product());
  static auto format = static_cast<OSPTextureFormat>(OSP_TEXTURE_RGB32F);
  static auto filter =
      static_cast<OSPTextureFilter>(OSP_TEXTURE_FILTER_BILINEAR);
  mapSh.set(skySize, skyImage.data(), format, filter);
}

SunSkyLight::~SunSkyLight()
{
  ispc::HDRILight_destroyDistribution(distributionIE);
}

ispc::Light *SunSkyLight::createSh(
    uint32_t index, const ispc::Instance *instance) const
{
  switch (index) {
  case 0: {
    ispc::HDRILight *sh = StructSharedCreate<ispc::HDRILight>();
    sh->set(visible,
        instance,
        coloredIntensity,
        frame,
        &mapSh,
        (const ispc::Distribution2D *)distributionIE);
    return &sh->super;
  }
  case 1: {
    ispc::DirectionalLight *sh = StructSharedCreate<ispc::DirectionalLight>();
    sh->set(visible, instance, direction, solarIrradiance, cosAngle);
    return &sh->super;
  }

  default:
    assert(false && "Incorrect SunSky sublight index");
  }
  return nullptr;
}

std::string SunSkyLight::toString() const
{
  return "ospray::SunSkyLight";
}

void SunSkyLight::commit()
{
  Light::commit();

  const float lambdaMin = 320.0f;
  const float lambdaMax = 720.0f;

  const vec3f up = normalize(getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f)));
  direction = -normalize(getParam<vec3f>("direction", vec3f(0.f, -1.f, 0.f)));
  const float albedo = clamp(getParam<float>("albedo", 0.3f), 0.1f, 1.f);
  const float turbidity = clamp(getParam<float>("turbidity", 3.f), 1.f, 10.f);
  const float horizon =
      clamp(getParam<float>("horizonExtension", 0.01f), 0.0f, 1.f);
  const float sunTheta = dot(up, direction);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();

  frame.vz = up;
  if (std::abs(sunTheta) > 0.99f) {
    const vec3f dx0 = vec3f(0.0f, up.z, -up.y);
    const vec3f dx1 = vec3f(-up.z, 0.0f, up.x);
    frame.vx = normalize(std::abs(up.x) < std::abs(up.y) ? dx0 : dx1);
    frame.vy = cross(up, frame.vx);
  } else {
    frame.vy = normalize(cross(-direction, frame.vz));
    frame.vx = cross(frame.vy, frame.vz);
  }

  // clamp sun to horizon
  if (sunTheta < 0)
    direction = frame.vx;

  // sun doesn't go beneath the horizon as theta clamped to pi/2
  const float sunThetaMax = min(std::acos(sunTheta), (float)pi * 0.999f / 2.0f);
  const float sunPhi = pi;
  const float sunElevation = (float)pi / 2.0f - sunThetaMax;

  ArHosekSkyModelState *spectralModel =
      arhosekskymodelstate_alloc_init(sunElevation, turbidity, albedo);

  // angular diameter of the sun in degrees
  // using this value produces matching solar irradiance results from the model
  // and directional light
  const float angularDiameter = 0.53;
  solarIrradiance = zero;

  // calculate solar radiance
  for (int i = 0; i < cieSize; ++i) {
    if (cieLambda[i] >= lambdaMin && cieLambda[i] <= lambdaMax) {
      float r = arhosekskymodel_solar_radiance_internal2(
          spectralModel, cieLambda[i], sunElevation, 1);
      solarIrradiance += r * cieXyz(i);
    }
  }

  arhosekskymodelstate_free(spectralModel);

  cosAngle = std::cos(deg2rad(0.5f * angularDiameter));
  const float rcpPdf = 2 * (float)pi * (1 - cosAngle);

  // convert solar radiance to solar irradiance
  solarIrradiance =
      xyzToRgb(solarIrradiance) * rcpPdf * intensityScale * coloredIntensity;

  ArHosekSkyModelState *rgbModel =
      arhosek_rgb_skymodelstate_alloc_init(turbidity, albedo, sunElevation);

  tasking::parallel_for(skySize.y, [&](int y) {
    for (int x = 0; x < skySize.x; x++) {
      float theta = (y + 0.5) / skySize.y * float(pi);
      const size_t index = skySize.x * y + x;
      vec3f skyRadiance = zero;

      const float maxTheta = 0.999 * float(pi) / 2.0;
      const float maxThetaHorizon = (horizon + 1.0) * float(pi) / 2.0;

      if (theta <= maxThetaHorizon) {
        float shadow = (horizon > 0.f)
            ? float(
                clamp((maxThetaHorizon - theta) / (maxThetaHorizon - maxTheta),
                    0.f,
                    1.f))
            : 1.f;
        theta = min(theta, maxTheta);

        float phi = ((x + 0.5) / skySize.x - 0.5) * (2.0 * (float)pi);

        float cosGamma = cos(theta) * cos(sunThetaMax)
            + sin(theta) * sin(sunThetaMax) * cos(phi - sunPhi);

        float gamma = acos(clamp(cosGamma, -1.f, 1.f));

        float rgbData[3];
        for (int i = 0; i < 3; ++i) {
          rgbData[i] =
              arhosek_tristim_skymodel_radiance(rgbModel, theta, gamma, i);
        }
        skyRadiance = vec3f(rgbData[0], rgbData[1], rgbData[2]);
        skyRadiance = skyRadiance * shadow;
        skyRadiance *= intensityScale;
      }
      skyImage[index] = max(skyRadiance, vec3f(0.0f));
    }
  });
  arhosekskymodelstate_free(rgbModel);

  // recreate distribution
  ispc::HDRILight_destroyDistribution(distributionIE);
  distributionIE = ispc::HDRILight_createDistribution(&mapSh);
}

void SunSkyLight::processIntensityQuantityType()
{
  // validate the correctness of the light quantity type
  if (intensityQuantity == OSP_INTENSITY_QUANTITY_SCALE) {
    coloredIntensity = getParam<vec3f>("color", vec3f(1.f));
    intensityScale = getParam<float>("intensity", 0.025f);
  } else if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
    coloredIntensity = getParam<vec3f>("color", vec3f(1.f));
    intensityScale = 0.025f * getParam<float>("intensity", 1.0f);
  } else {
    postStatusMsg(OSP_LOG_WARNING)
        << toString() << " unsupported 'intensityQuantity' value";
    coloredIntensity = vec3f(0.0f);
  }
}

} // namespace ospray
