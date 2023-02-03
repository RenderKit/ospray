// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistribution.h"
#include "Light.h"

namespace ospray {

/*! a SpotLight is a singular light emitting from a point uniformly into a
 *  cone of directions bounded by halfAngle */
struct OSPRAY_SDK_INTERFACE SpotLight : public Light
{
  SpotLight(api::ISPCDevice &device) : Light(device, FFO_LIGHT_SPOT) {}
  virtual ~SpotLight() override = default;
  virtual ISPCRTMemoryView createSh(
      uint32_t, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType(const float openingAngle);

  vec3f position{0.f};
  vec3f direction{0.f, 0.f, 1.f};
  vec3f radiance{1.f};
  vec3f radIntensity{0.f};
  float radius{0.f};
  float innerRadius{0.f};
  float cosAngleMax{1.f};
  float cosAngleScale{1.f};
  IntensityDistribution intensityDistribution;
};

} // namespace ospray
