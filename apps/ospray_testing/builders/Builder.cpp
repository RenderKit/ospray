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
        tfColorMap   = getParam<std::string>("tf.colorMap", "jet");
        tfOpacityMap = getParam<std::string>("tf.opacityMap", "linear");
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

      cpp::TransferFunction Builder::makeTransferFunction(
          const vec2f &valueRange) const
      {
        cpp::TransferFunction transferFunction("piecewise_linear");

        std::vector<vec3f> colors;
        std::vector<float> opacities;

        if (tfColorMap == "jet") {
          colors.emplace_back(0, 0, 0.562493);
          colors.emplace_back(0, 0, 1);
          colors.emplace_back(0, 1, 1);
          colors.emplace_back(0.500008, 1, 0.500008);
          colors.emplace_back(1, 1, 0);
          colors.emplace_back(1, 0, 0);
          colors.emplace_back(0.500008, 0, 0);
        } else if (tfColorMap == "rgb") {
          colors.emplace_back(0, 0, 1);
          colors.emplace_back(0, 1, 0);
          colors.emplace_back(1, 0, 0);
        } else {
          colors.emplace_back(0.f, 0.f, 0.f);
          colors.emplace_back(1.f, 1.f, 1.f);
        }

        if (tfOpacityMap == "linear") {
          opacities.emplace_back(0.f);
          opacities.emplace_back(1.f);
        }

        transferFunction.setParam(
            "color", cpp::Data(colors.size(), OSP_VEC3F, colors.data()));
        transferFunction.setParam(
            "opacity",
            cpp::Data(opacities.size(), OSP_FLOAT, opacities.data()));
        transferFunction.setParam("valueRange", valueRange);
        transferFunction.commit();

        return transferFunction;
      }

    }  // namespace detail
  }    // namespace testing
}  // namespace ospray
