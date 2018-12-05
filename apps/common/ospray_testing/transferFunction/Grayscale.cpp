// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

    struct Grayscale : public TransferFunction
    {
      Grayscale()           = default;
      ~Grayscale() override = default;

      OSPTransferFunction createTransferFunction(
          osp::vec2f value_range) const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTransferFunction Grayscale::createTransferFunction(
        osp::vec2f value_range) const
    {
      // create a transfer function mapping volume values to color and opacity
      OSPTransferFunction transferFunction =
          ospNewTransferFunction("piecewise_linear");

      std::vector<vec3f> transferFunctionColors;
      std::vector<float> transferFunctionOpacities;

      transferFunctionColors.push_back(vec3f(0.f, 0.f, 0.f));
      transferFunctionColors.push_back(vec3f(1.f, 1.f, 1.f));

      transferFunctionOpacities.push_back(0.f);
      transferFunctionOpacities.push_back(0.1f);

      OSPData transferFunctionColorsData =
          ospNewData(transferFunctionColors.size(),
                     OSP_FLOAT3,
                     transferFunctionColors.data());
      OSPData transferFunctionOpacitiesData =
          ospNewData(transferFunctionOpacities.size(),
                     OSP_FLOAT,
                     transferFunctionOpacities.data());

      ospSetData(transferFunction, "colors", transferFunctionColorsData);
      ospSetData(transferFunction, "opacities", transferFunctionOpacitiesData);

      // the transfer function will apply over this volume value range
      ospSet2f(transferFunction, "valueRange", value_range.x, value_range.y);

      // commit the transfer function
      ospCommit(transferFunction);

      return transferFunction;
    }

    OSP_REGISTER_TESTING_TRANSFER_FUNCTION(Grayscale, grayscale);

  }  // namespace testing
}  // namespace ospray
