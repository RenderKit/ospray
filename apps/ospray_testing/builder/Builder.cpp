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

namespace ospray {
  namespace testing {
    namespace detail {

      void Builder::commit()
      {
        rendererType = getParam<std::string>("rendererType", "scivis");
      }

      cpp::World Builder::buildWorld() const
      {
        cpp::World world;

        auto group = buildGroup();

        cpp::Instance inst(group);
        inst.commit();

        world.setParam("instance", cpp::Data(inst));

        cpp::Light light("ambient");
        light.commit();

        world.setParam("light", cpp::Data(light));

        return world;
      }

    }  // namespace detail
  }    // namespace testing
}  // namespace ospray
