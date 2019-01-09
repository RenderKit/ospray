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

using namespace ospcommon;

namespace ospray {
  namespace testing {

    TransferFunction::TransferFunction()
    {
      opacities.emplace_back(0.0f);
      opacities.emplace_back(0.1f);
    }

    OSPTransferFunction TransferFunction::createTransferFunction(
        osp::vec2f value_range) const
    {
      // create a transfer function mapping volume values to color and opacity
      OSPTransferFunction transferFunction =
          ospNewTransferFunction("piecewise_linear");

      OSPData cData = ospNewData(colors.size(), OSP_FLOAT3, colors.data());
      OSPData oData = ospNewData(opacities.size(), OSP_FLOAT, opacities.data());

      ospSetData(transferFunction, "colors", cData);
      ospSetData(transferFunction, "opacities", oData);

      // the transfer function will apply over this volume value range
      ospSet2f(transferFunction, "valueRange", value_range.x, value_range.y);

      // commit the transfer function
      ospCommit(transferFunction);

      return transferFunction;
    }

  }  // namespace testing
}  // namespace ospray
