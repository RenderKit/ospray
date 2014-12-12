// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

  //!< Base class for spot light objects
  struct SpotLight : public Light {

    //!< Constructor for SpotLight.
    SpotLight();

    //!< Copy understood parameters into class members
    virtual void commit();

    //!< toString is used to aid in printf debugging
    virtual std::string toString() const { return "ospray::SpotLight"; }

    vec3f position;         //!< Position of the SpotLight
    vec3f direction;        //!< Direction that the SpotLight is pointing
    vec3f color;            //!< RGB color of the SpotLight
    float range;            //!< Max influence range of the SpotLight
    float halfAngle;        //!< Half angle of spot light. If angle from intersection to light is greater than this, the light does not influence shading for that intersection
    float angularDropOff;   //!< This gives the drop off of light intensity as angle between intersection point and light position increases

  };

}
