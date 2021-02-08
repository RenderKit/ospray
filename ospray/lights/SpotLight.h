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
  SpotLight();
  virtual ~SpotLight() override = default;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType(const float &radius,
      const float &innerRadius,
      const float &openingAngle,
      vec3f &radIntensity);

  Ref<const DataT<float, 2>> lid; // luminous intensity distribution
  vec3f radiance{1.0f, 1.0f, 1.0f};
};

} // namespace ospray
