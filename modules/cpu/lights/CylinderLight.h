// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

/*! a CylinderLight is a virtual area light uniformly emitting from a
 * cylindrical area into outward space */
struct OSPRAY_SDK_INTERFACE CylinderLight : public Light
{
  CylinderLight(api::ISPCDevice &device) : Light(device, FFO_LIGHT_CYLINDER) {}
  virtual ~CylinderLight() override = default;
  virtual ISPCRTMemoryView createSh(
      uint32_t, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  vec3f position0{0.f}; // start of the cylinder
  vec3f position1{0.f, 0.f, 1.f}; // end of the cylinder
  vec3f radiance{1.0f, 1.0f, 1.0f};
  float radius{1.f};
};

} // namespace ospray
