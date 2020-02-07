// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"
#include "texture/Texture2D.h"

namespace ospray {

/*! a SpotLight is a singular light emitting from a point uniformly into a
 *  cone of directions bounded by halfAngle */
struct OSPRAY_SDK_INTERFACE HDRILight : public Light
{
  HDRILight();
  virtual ~HDRILight() override;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  vec3f up{0.f, 1.f, 0.f}; //!< up direction of the light in world-space
  vec3f dir{0.f, 0.f, 1.f}; //!< direction to which the center of the envmap
                            //   will be mapped to (analog to panoramic camera)
  Texture2D *map{nullptr}; //!< environment map in latitude / longitude format
};

} // namespace ospray
