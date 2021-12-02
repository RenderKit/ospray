// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

/*! a CylinderLight is a virtual area light uniformly emitting from a
 * cylindrical area into outward space */
struct OSPRAY_SDK_INTERFACE CylinderLight : public Light
{
  CylinderLight();
  virtual ~CylinderLight() override = default;
  virtual void *createIE(const void *instance) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  vec3f position0{0.f}; //!< world-space bottom position of the light
  vec3f position1{0.f, 0.f, 1.f}; //!< world-space top position of the light
  vec3f radiance{1.0f, 1.0f, 1.0f}; //!< emitted radiance of the CylinderLight
  float radius{1.f}; //!< vector along the radius of the cylinder
};

} // namespace ospray
