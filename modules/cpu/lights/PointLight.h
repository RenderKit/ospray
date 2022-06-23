// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistribution.h"
#include "Light.h"

namespace ospray {

//! a PointLight is a singular light emitting from a point uniformly into all
//! directions
struct OSPRAY_SDK_INTERFACE PointLight : public Light
{
  PointLight() = default;
  virtual ~PointLight() override = default;
  virtual void *createIE(const void *instance) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  vec3f position{0.f}; //!< world-space position of the light
  float radius{0.f}; //!< Radius of SphereLight
  vec3f radiance{1.f}; //!< emitted radiance of the SphereLight
  vec3f radIntensity{0.f};
  IntensityDistribution intensityDistribution;
  vec3f direction{0.f, 0.f, 1.f};
};

} // namespace ospray
