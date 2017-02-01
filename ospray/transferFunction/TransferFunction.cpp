// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// ospray
#include "transferFunction/TransferFunction.h"
#include "common/Util.h"
#include "TransferFunction_ispc.h"

namespace ospray {

  void TransferFunction::commit()
  {
    vec2f valueRange = getParam2f("valueRange", vec2f(0.0f, 1.0f));
    ispc::TransferFunction_setValueRange(ispcEquivalent,
                                         (const ispc::vec2f &)valueRange);
  }

  TransferFunction *TransferFunction::createInstance(const std::string &type)
  {
    return createInstanceHelper<TransferFunction, OSP_TRANSFER_FUNCTION>(type);
  }

  std::string TransferFunction::toString() const
  {
    return "ospray::TransferFunction";
  }

} // ::ospray

