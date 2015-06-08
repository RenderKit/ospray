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
#include "SpotLight_ispc.h"

#ifdef _WIN32
#  define _USE_MATH_DEFINES
#  include <math.h> // M_PI
#endif

namespace ospray {
  SpotLight::SpotLight()
    : position(0.f)
    , direction(0.f, 0.f, 1.f)
    , color(1.f)
    , intensity(1.f)
    , halfAngle(90.f)
    , range(inf)
  {
    ispcEquivalent = ispc::SpotLight_create(this);
  }

  //!< Copy understood parameters into class members
  void SpotLight::commit() {
    position  = getParam3f("position", vec3f(0.f));
    direction = getParam3f("direction", vec3f(0.f, 0.f, 1.f));
    color     = getParam3f("color", vec3f(1.f));
    intensity = getParam1f("intensity", 1.f);
    halfAngle = getParam1f("halfAngle", 90.f);
    range     = getParam1f("range", inf);

    vec3f power = color * intensity;
    
    ispc::SpotLight_set(getIE(),
                        (ispc::vec3f&)position,
                        (ispc::vec3f&)direction,
                        (ispc::vec3f&)power,
                        cos(halfAngle * (M_PI / 180.0f)),
                        range);
  }

  OSP_REGISTER_LIGHT(SpotLight, SpotLight);
}
