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

#include "../ospray_testing.h"

namespace ospray {
  namespace testing {

    struct Lights
    {
      virtual ~Lights() = default;

      virtual OSPData createLights() const = 0;
    };

  }  // namespace testing
}  // namespace ospray

#define OSP_REGISTER_TESTING_LIGHTS(InternalClassName, Name)                 \
  OSPRAY_TESTING_DLLEXPORT                                                   \
  extern "C" ospray::testing::Lights *ospray_create_testing_lights__##Name() \
  {                                                                          \
    return new InternalClassName;                                            \
  }                                                                          \
  /* Extra declaration to avoid "extra ;" pedantic warnings */               \
  ospray::testing::Lights *ospray_create_testing_lights__##Name()
