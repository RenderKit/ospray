// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

/*! a DirectionalLight is a singular light which is infinitely distant and
 *  thus projects parallel rays of light across the entire scene */
struct OSPRAY_SDK_INTERFACE DirectionalLight : public Light
{
  DirectionalLight() = default;
  virtual ~DirectionalLight() override = default;
  virtual void *createIE(const void *instance) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  vec3f irradiance{1.f};
  vec3f direction{0.f, 0.f, 1.f}; //!< Direction of the emitted rays
  float angularDiameter{0.f}; //!< Apparent size of the distant light, in degree
                              //!< (e.g. 0.53 for the sun)
  float cosAngle{1.f};
};

} // namespace ospray
