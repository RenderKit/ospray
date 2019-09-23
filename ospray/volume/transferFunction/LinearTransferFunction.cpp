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

#include "LinearTransferFunction.h"
#include "LinearTransferFunction_ispc.h"

namespace ospray {

  LinearTransferFunction::LinearTransferFunction()
  {
    ispcEquivalent = ispc::LinearTransferFunction_create();
  }

  void LinearTransferFunction::commit()
  {
    TransferFunction::commit();

    colorValues = getParamDataT<vec3f>("color");
    opacityValues = getParamDataT<float>("opacity");

    ispc::LinearTransferFunction_set(
        ispcEquivalent, ispc(colorValues), ispc(opacityValues));
  }

  std::string LinearTransferFunction::toString() const
  {
    return "ospray::LinearTransferFunction";
  }

  OSP_REGISTER_TRANSFER_FUNCTION(LinearTransferFunction, piecewise_linear);

} // namespace ospray
