// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

//! an AmbientLight is a constant light that is present everywhere
struct OSPRAY_SDK_INTERFACE AmbientLight : public Light
{
  AmbientLight() = default;
  virtual ~AmbientLight() override = default;
  virtual void *createIE(const void *instance) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

  vec3f radiance{1.f}; //!< emitted radiance of the AmbientLight

 private:
  void processIntensityQuantityType();
};

} // namespace ospray
