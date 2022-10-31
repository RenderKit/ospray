// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "texture/Texture2D.h"

namespace ospray {

/*! a SpotLight is a singular light emitting from a point uniformly into a
 *  cone of directions bounded by halfAngle */
struct OSPRAY_SDK_INTERFACE HDRILight : public Light
{
  HDRILight() = default;
  virtual ~HDRILight() override;
  virtual ispc::Light *createSh(
      uint32_t, const ispc::Instance *instance = nullptr) const override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  void processIntensityQuantityType();

  linear3f frame{one}; // light orientation
  vec3f position{0.0f};
  float radius{10.0f};
  Texture2D *map{nullptr}; //!< environment map in latitude / longitude format
  void *distributionIE{nullptr};
};

} // namespace ospray
