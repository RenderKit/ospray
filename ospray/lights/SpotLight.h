// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
    vec3f position {0.f};           //!< world-space position of the light
    vec3f direction {0.f, 0.f, 1.f};//!< Direction that the SpotLight is pointing
    vec3f color {1.f};              //!< RGB color of the SpotLight
    float intensity {1.f};          //!< Amount of light emitted
    float openingAngle {180.f};     //!< Full opening angle of spot light, in degree. If angle from hit to light is greater than 1/2 * this, the light does not influence shading for that point
    float penumbraAngle {5.f};      //!< Angle, in degree, of the "penumbra", the region between the rim and full intensity of the spot. Should be smaller than half of the openingAngle.
    float radius {0.f};             //!< Radius of ExtendedSpotLight
  };

}
