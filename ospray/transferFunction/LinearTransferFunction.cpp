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

#include "transferFunction/LinearTransferFunction.h"
#include "LinearTransferFunction_ispc.h"

namespace ospray {

  void LinearTransferFunction::commit()
  {
    // Create the equivalent ISPC transfer function.
    if (ispcEquivalent == nullptr) createEquivalentISPC();

    // Retrieve the color and opacity values.
    colorValues   = getParamData("colors", nullptr);
    opacityValues = getParamData("opacities", nullptr);
    ispc::LinearTransferFunction_setPreIntegration(ispcEquivalent,
                                               getParam1i("preIntegration", 0));

    // Set the color values.
    if (colorValues) {
      ispc::LinearTransferFunction_setColorValues(ispcEquivalent, 
                                                  colorValues->numItems, 
                                                  (ispc::vec3f*)colorValues->data);
    }

    // Set the opacity values.
    if (opacityValues) {
      ispc::LinearTransferFunction_setOpacityValues(ispcEquivalent, 
                                                    opacityValues->numItems, 
                                                    (float *)opacityValues->data);
    }

    if (getParam1i("preIntegration", 0) && colorValues && opacityValues)
      ispc::LinearTransferFunction_precomputePreIntegratedValues(ispcEquivalent);

    TransferFunction::commit();

    // Notify listeners that the transfer function has changed.
    notifyListenersThatObjectGotChanged();
  }

  std::string LinearTransferFunction::toString() const
  {
    return "ospray::LinearTransferFunction";
  }

  void LinearTransferFunction::createEquivalentISPC()
  {
    // The equivalent ISPC transfer function must not exist yet.
    exitOnCondition(ispcEquivalent != nullptr,
                    "attempt to overwrite an existing ISPC transfer function");

    // Create the equivalent ISPC transfer function.
    ispcEquivalent = ispc::LinearTransferFunction_createInstance();

    // The object may not have been created.
    exitOnCondition(ispcEquivalent == nullptr,
                    "unable to create ISPC transfer function");
  }

  // A piecewise linear transfer function.
  OSP_REGISTER_TRANSFER_FUNCTION(LinearTransferFunction, piecewise_linear);

} // ::ospray

