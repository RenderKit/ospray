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

#include <vector>
#include "Lights.h"

namespace ospray {
  namespace testing {

    struct AmbientAndDirectional : public Lights
    {
      ~AmbientAndDirectional() override = default;

      OSPData createLights() const override;
    };

    OSPData AmbientAndDirectional::createLights() const
    {
      std::vector<OSPLight> lights;

      auto ambientLight = ospNewLight3("ambient");
      ospSet1f(ambientLight, "intensity", 0.2f);
      ospSet3f(ambientLight, "color", 1.f, 1.f, 1.f);
      ospCommit(ambientLight);
      lights.push_back(ambientLight);

      auto directionalLight = ospNewLight3("distant");
      ospSet1f(directionalLight, "intensity", 0.9f);
      ospSet3f(directionalLight, "color", 1.f, 1.f, 1.f);
      ospSet3f(directionalLight, "direction", 0.f, -1.f, 0.f);
      ospCommit(directionalLight);
      lights.push_back(directionalLight);

      auto lightsData = ospNewData(lights.size(), OSP_OBJECT, lights.data());

      ospRelease(ambientLight);
      ospRelease(directionalLight);

      return lightsData;
    }

    OSP_REGISTER_TESTING_LIGHTS(AmbientAndDirectional, ambient_and_directional);

  }  // namespace testing
}  // namespace ospray
