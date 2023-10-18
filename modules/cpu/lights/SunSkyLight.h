// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "math/Distribution2D.h"
#include "math/spectrum.h"
#include "rkcommon/tasking/parallel_for.h"
#include "sky_model/color_info.h"
#include "sky_model/sky_model.h"
// ispc shared
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
  SunSkyLight(api::ISPCDevice &device);
  virtual uint32_t getShCount() const override;
  virtual ISPCRTMemoryView createSh(
      uint32_t index, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  std::unique_ptr<BufferShared<vec3f>> skyImage;
  Ref<Texture2D> map;
  Ref<Distribution2D> distribution;
  vec2i skySize;
  linear3f frame{one}; // sky orientation
  vec3f direction{0.f, 0.f, 1.f};
  vec3f solarIrradiance{0.f};
  float cosAngle{1.f};
  // scaling factor for values provided from model for both sun and sky to be <1
  float intensityScale{0.025f};
};

// Inlined defintions /////////////////////////////////////////////////////////

inline uint32_t SunSkyLight::getShCount() const
{
  return 2;
}

} // namespace ospray
