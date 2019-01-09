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
// ospcommon
#include "ospcommon/vec.h"
// stl
#include <string>
#include <vector>

namespace ospray {
  namespace testing {

    struct TransferFunction
    {
      TransferFunction();
      virtual ~TransferFunction() = default;

      OSPTransferFunction createTransferFunction(osp::vec2f value_range) const;

     protected:
      std::vector<ospcommon::vec3f> colors;
      std::vector<float> opacities;
    };

  }  // namespace testing
}  // namespace ospray

#define OSP_REGISTER_TESTING_TRANSFER_FUNCTION(InternalClassName, Name) \
  OSPRAY_TESTING_DLLEXPORT                                              \
  extern "C" ospray::testing::TransferFunction                          \
      *ospray_create_testing_transfer_function__##Name()                \
  {                                                                     \
    return new InternalClassName;                                       \
  }                                                                     \
  /* Extra declaration to avoid "extra ;" pedantic warnings */          \
  ospray::testing::TransferFunction                                     \
      *ospray_create_testing_transfer_function__##Name()
