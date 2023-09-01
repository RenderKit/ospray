// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "math/Distribution2D.h"
#include "texture/Texture2D.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE HDRILight : public Light
{
  HDRILight(api::ISPCDevice &device) : Light(device, FFO_LIGHT_HDRI) {}
  virtual ISPCRTMemoryView createSh(
      uint32_t, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  linear3f frame{one}; // light orientation
  Ref<Texture2D> map;
  Ref<Distribution2D> distribution;
};

} // namespace ospray
