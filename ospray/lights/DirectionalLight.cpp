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

#include "DirectionalLight.h"
#include "DirectionalLight_ispc.h"

namespace ospray {

  DirectionalLight::DirectionalLight()
  {
    ispcEquivalent = ispc::DirectionalLight_create();
  }

  std::string DirectionalLight::toString() const
  {
    return "ospray::DirectionalLight";
  }

  void DirectionalLight::commit()
  {
    Light::commit();
    direction = getParam3f("direction", vec3f(0.f, 0.f, 1.f));
    angularDiameter = getParam1f("angularDiameter", .0f);

    // the ispc::DirLight expects direction towards light source
    direction = -normalize(direction);

    angularDiameter      = clamp(angularDiameter, 0.f, 180.f);
    const float cosAngle = ospcommon::cos(deg2rad(0.5f * angularDiameter));

    ispc::DirectionalLight_set(getIE(), (ispc::vec3f &)direction, cosAngle);
  }

  OSP_REGISTER_LIGHT(DirectionalLight, DirectionalLight);
  OSP_REGISTER_LIGHT(DirectionalLight, DistantLight);
  OSP_REGISTER_LIGHT(DirectionalLight, distant);
  OSP_REGISTER_LIGHT(DirectionalLight, directional);

} // ::ospray
