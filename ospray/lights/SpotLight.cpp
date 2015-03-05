// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "SpotLight.h"

namespace ospray {
  //!< Constructor for SpotLight. Placed in protected section to make
  //!SpotLight act as an abstract base class.
  SpotLight::SpotLight()
    : position(0.f, 0.f, 0.f)
    , direction(1.f, 1.f, 1.f)
    , color(1.f, 1.f, 1.f)
    , range(-1.f)
    , halfAngle(-1.f)
  {}

  //!< Copy understood parameters into class members
  void SpotLight::commit() {
    position  = getParam3f("position", vec3f(0.f, 0.f, 0.f));
    direction = getParam3f("direction", vec3f(1.f, 1.f, 1.f));
    color = getParam3f("color", vec3f(1.f, 1.f, 1.f));
    range = getParam1f("range", -1.f);
    halfAngle = getParam1f("halfAngle", -1.f);
  }
}
