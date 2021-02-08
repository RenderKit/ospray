// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

//! a PointLight is a singular light emitting from a point uniformly into all
//! directions
struct OSPRAY_SDK_INTERFACE PointLight : public Light
{
  PointLight();
  virtual ~PointLight() override = default;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType(vec3f &radIntensity);

  vec3f position{0.f}; //!< world-space position of the light
  float radius{0.f}; //!< Radius of SphereLight
  vec3f radiance{1.0f, 1.0f, 1.0f}; //!< emitted radiance of the SphereLight
};

} // namespace ospray
