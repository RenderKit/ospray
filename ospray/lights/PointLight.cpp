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

#include "PointLight.h"

namespace ospray {
  //!Construct a new PointLight object
  PointLight::PointLight()
    : position(0.f, 0.f, 0.f)
    , color(1.f, 1.f, 1.f)
    , range(-1.f)
  {
  }

  //!Commit parameters understood by the base PointLight class
  void PointLight::commit() {
    position = getParam3f("position", vec3f(0.f, 0.f, 0.f));
    color    = getParam3f("color", vec3f(1.f, 1.f, 1.f));
    range    = getParam1f("range", -1.f);
  }
}
