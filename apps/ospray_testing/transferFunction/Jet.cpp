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

#include "TransferFunction.h"
// stl
#include <vector>
// ospcommon
#include "ospcommon/box.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct Jet : public TransferFunction
    {
      Jet();
      ~Jet() override = default;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    Jet::Jet()
    {
      colors.emplace_back(0       , 0, 0.562493);
      colors.emplace_back(0       , 0, 1       );
      colors.emplace_back(0       , 1, 1       );
      colors.emplace_back(0.500008, 1, 0.500008);
      colors.emplace_back(1       , 1, 0       );
      colors.emplace_back(1       , 0, 0       );
      colors.emplace_back(0.500008, 0, 0       );
    }

    OSP_REGISTER_TESTING_TRANSFER_FUNCTION(Jet, jet);

  }  // namespace testing
}  // namespace ospray
