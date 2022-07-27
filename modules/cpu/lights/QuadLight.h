// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistribution.h"
#include "Light.h"

namespace ospray {

/*! a QuadLight is a virtual area light uniformly emitting from a rectangular
 * area into the positive half space */
struct OSPRAY_SDK_INTERFACE QuadLight : public Light
{
  QuadLight() = default;
  virtual ~QuadLight() override = default;
  virtual ispc::Light *createSh(
      uint32_t, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  vec3f position{0.f}; //!< world-space corner position of the light
  vec3f edge1{1.f, 0.f, 0.f}; //!< vectors to adjacent corners
  vec3f edge2{0.f, 1.f, 0.f}; //!< vectors to adjacent corners
  vec3f radiance{1.f}; //!< emitted radiance of the QuadLight
  IntensityDistribution intensityDistribution;
};

} // namespace ospray
