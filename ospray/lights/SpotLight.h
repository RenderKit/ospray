// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Light.h"

namespace ospray {

/*! a SpotLight is a singular light emitting from a point uniformly into a
 *  cone of directions bounded by halfAngle */
struct OSPRAY_SDK_INTERFACE SpotLight : public Light
{
  SpotLight();
  virtual ~SpotLight() override = default;
  virtual std::string toString() const override;
  virtual void commit() override;

 private:
  vec3f position{0.f}; //!< world-space position of the light
  vec3f direction{0.f, 0.f, 1.f}; //!< Direction that the SpotLight is pointing
  float openingAngle{
      180.f}; //!< Full opening angle of spot light, in degree. If angle from
              //!< hit to light is greater than 1/2 * this, the light does not
              //!< influence shading for that point
  float penumbraAngle{
      5.f}; //!< Angle, in degree, of the "penumbra", the region between the rim
            //!< and full intensity of the spot. Should be smaller than half of
            //!< the openingAngle.
  float radius{0.f}; //!< Radius of ExtendedSpotLight
};

} // namespace ospray
