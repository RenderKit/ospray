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

// ospray
#include "../ospray_testing.h"
// stl
#include <string>

namespace ospray {
  namespace testing {

    struct Geometry
    {
      virtual ~Geometry() = default;

      virtual OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const = 0;
    };

  }  // namespace testing
}  // namespace ospray

#define OSP_REGISTER_TESTING_GEOMETRY(InternalClassName, Name) \
  OSPRAY_TESTING_DLLEXPORT                                     \
  extern "C" ospray::testing::Geometry                         \
      *ospray_create_testing_geometry__##Name()                \
  {                                                            \
    return new InternalClassName;                              \
  }                                                            \
  /* Extra declaration to avoid "extra ;" pedantic warnings */ \
  ospray::testing::Geometry *ospray_create_testing_geometry__##Name()
