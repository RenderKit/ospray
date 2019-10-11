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

#include "Builder.h"
#include "ospray_testing.h"

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Empty : public detail::Builder
    {
      Empty()           = default;
      ~Empty() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Empty::commit()
    {
      Builder::commit();
      addPlane = false;
    }

    cpp::Group Empty::buildGroup() const
    {
      cpp::Group group;
      group.commit();
      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Empty, empty);

  }  // namespace testing
}  // namespace ospray
