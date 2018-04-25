// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "QuadLight.h"
#include "QuadLight_ispc.h"

namespace ospray {

  QuadLight::QuadLight()
  {
    ispcEquivalent = ispc::QuadLight_create();
  }

  std::string QuadLight::toString() const
  {
    return "ospray::QuadLight";
  }

  void QuadLight::commit()
  {
    Light::commit();
    position  = getParam3f("position", vec3f(0.f));
    edge1     = getParam3f("edge1", vec3f(1.f, 0.f, 0.f));
    edge2     = getParam3f("edge2", vec3f(0.f, 1.f, 0.f));
    color     = getParam3f("color", vec3f(1.f));
    intensity = getParam1f("intensity", 1.f);

    vec3f radiance = color * intensity;

    ispc::QuadLight_set(getIE(),
                        (ispc::vec3f&)position,
                        (ispc::vec3f&)edge1,
                        (ispc::vec3f&)edge2,
                        (ispc::vec3f&)radiance);
  }

  OSP_REGISTER_LIGHT(QuadLight, QuadLight);
  OSP_REGISTER_LIGHT(QuadLight, quad); // actually a parallelogram

}// ::ospray
