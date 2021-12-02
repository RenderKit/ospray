// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "common/Data.h"

namespace ospray {

/*! a SpotLight is a singular light emitting from a point uniformly into a
 *  cone of directions bounded by halfAngle */
struct OSPRAY_SDK_INTERFACE SpotLight : public Light
{
  SpotLight() = default;
  virtual ~SpotLight() override = default;
  virtual void *createIE(const void *instance) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType(const float openingAngle);

  Ref<const DataT<float, 2>> lid; // luminous intensity distribution
  vec2i size{0};
  vec3f position{0.f};
  vec3f direction{0.f, 0.f, 1.f};
  vec3f c0{1.f, 0.f, 0.f};
  vec3f radiance{1.f};
  vec3f radIntensity{0.f};
  float radius{0.f};
  float innerRadius{0.f};
  float cosAngleMax{1.f};
  float cosAngleScale{1.f};
};

} // namespace ospray
