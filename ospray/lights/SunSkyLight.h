// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "sky_model/color_info.h"
#include "sky_model/sky_model.h"
#include "math/spectrum.h"
#include "ospcommon/tasking/parallel_for.h"
#include "texture/Texture2D.h"

// Sun and sky environment lights
// [Hosek and Wilkie 2012, "An Analytic Model for Full Spectral Sky-Dome
// Radiance"] [Hosek and Wilkie 2013, "Adding a Solar Radiance Function to the
// Hosek Skylight Model"]

namespace ospray {

// // CIE XYZ color matching functions
// // --------------------------------
inline vec3f cieXyz(int i)
{
  return vec3f(cieX[i], cieY[i], cieZ[i]);
}

inline vec3f xyzToRgb(const vec3f &c)
{
  float r = 3.240479f * c.x - 1.537150f * c.y - 0.498535f * c.z;
  float g = -0.969256f * c.x + 1.875991f * c.y + 0.041556f * c.z;
  float b = 0.055648f * c.x - 0.204043f * c.y + 1.057311f * c.z;

  return vec3f(r, g, b);
}

struct OSPRAY_SDK_INTERFACE SunSkyLight : public Light
{
  SunSkyLight();
  virtual ~SunSkyLight() override;
  virtual std::string toString() const override;
  virtual void commit() override;
  virtual utility::Optional<void *> getSecondIE() override;

 private:
  vec3f direction; // direction of the sun
  float turbidity{3.0f}; // turbidity must be between 1 and 10
  float albedo; // albedo of the ground
  vec3f up; // up direction of the sky
  std::vector<vec3f> skyImage;
  Texture2D *map{nullptr};
  void *secondLightIE;
  vec2i skySize;
};

} // namespace ospray
