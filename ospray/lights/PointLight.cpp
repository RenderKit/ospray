// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "PointLight_ispc.h"

namespace ospray {
  PointLight::PointLight()
    : position(0.f)
    , color(1.f)
    , intensity(1.f)
    , radius(0.f)
  {
    ispcEquivalent = ispc::PointLight_create();
  }

  //! Commit parameters understood by the PointLight
  void PointLight::commit() {
    position  = getParam3f("position", vec3f(0.f));
    color     = getParam3f("color", vec3f(1.f));
    intensity = getParam1f("intensity", 1.f);
    radius    = getParam1f("radius", 0.f);

    vec3f power = color * intensity;

    ispc::PointLight_set(getIE(), (ispc::vec3f&)position, (ispc::vec3f&)power, radius);
  }

  OSP_REGISTER_LIGHT(PointLight, PointLight);
  OSP_REGISTER_LIGHT(PointLight, point);
  OSP_REGISTER_LIGHT(PointLight, SphereLight);
  OSP_REGISTER_LIGHT(PointLight, sphere);
}
