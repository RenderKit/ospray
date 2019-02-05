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

#include "SpotLight.h"
#include "SpotLight_ispc.h"

namespace ospray {

  SpotLight::SpotLight()
  {
    ispcEquivalent = ispc::SpotLight_create();
  }

  std::string SpotLight::toString() const
  {
    return "ospray::SpotLight";
  }

  void SpotLight::commit()
  {
    Light::commit();
    position  = getParam3f("position", vec3f(0.f));
    direction = getParam3f("direction", vec3f(0.f, 0.f, 1.f));
    color     = getParam3f("color", vec3f(1.f));
    intensity = getParam1f("intensity", 1.f);
    openingAngle = getParam1f("openingAngle", 2.0f * getParam1f("halfAngle"/*deprecated*/, 90.f));
    penumbraAngle = getParam1f("penumbraAngle", 5.f);
    radius    = getParam1f("radius", 0.f);

    // check ranges and pre-compute parameters
    vec3f power = color * intensity;
    direction = normalize(direction);
    openingAngle = clamp(openingAngle, 0.f, 180.f);
    penumbraAngle = clamp(penumbraAngle, 0.f, 0.5f*openingAngle);
    const float cosAngleMax = ospcommon::cos(deg2rad(0.5f*openingAngle));
    const float cosAngleMin = ospcommon::cos(deg2rad(0.5f*openingAngle - penumbraAngle));
    const float cosAngleScale = 1.0f/(cosAngleMin - cosAngleMax);

    ispc::SpotLight_set(getIE(),
                        (ispc::vec3f&)position,
                        (ispc::vec3f&)direction,
                        (ispc::vec3f&)power,
                        cosAngleMax,
                        cosAngleScale,
                        radius);
  }

  OSP_REGISTER_LIGHT(SpotLight, SpotLight);
  OSP_REGISTER_LIGHT(SpotLight, ExtendedSpotLight);
  OSP_REGISTER_LIGHT(SpotLight, spot);
}
