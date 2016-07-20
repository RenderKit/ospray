// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "common/Data.h"
#include "common/OSPCommon.h"
#include "transferFunction/LinearTransferFunction.h"
#include "TransferFunction_ispc.h"

namespace ospray {

    //! Destructor.
  LinearTransferFunction::~LinearTransferFunction() 
  {
    if (ispcEquivalent != NULL) 
      ispc::LinearTransferFunction_destroy(ispcEquivalent); 
  }

  void LinearTransferFunction::commit()
  {
    // Create the equivalent ISPC transfer function.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    // Retrieve the color and opacity values.
    colorValues   = getParamData("colors", NULL);  
    opacityValues = getParamData("opacities", NULL);

    // Set the color values.
    if (colorValues) 
      ispc::LinearTransferFunction_setColorValues(ispcEquivalent, 
                                                  colorValues->numItems, 
                                                  (ispc::vec3f *) colorValues->data);

    // Set the opacity values.
    if (opacityValues) {
      float *alpha = (float *)opacityValues->data;
      ispc::LinearTransferFunction_setOpacityValues(ispcEquivalent, 
                                                    opacityValues->numItems, 
                                                    (float *)opacityValues->data);
    }

    // Set the value range that the transfer function covers.
    vec2f valueRange = getParam2f("valueRange", vec2f(0.0f, 1.0f));  
    ispc::TransferFunction_setValueRange(ispcEquivalent, (const ispc::vec2f &) valueRange);

    // Notify listeners that the transfer function has changed.
    notifyListenersThatObjectGotChanged();
  }

  void LinearTransferFunction::createEquivalentISPC()
  {
    // The equivalent ISPC transfer function must not exist yet.
    exitOnCondition(ispcEquivalent != NULL, "attempt to overwrite an existing ISPC transfer function");

    // Create the equivalent ISPC transfer function.
    ispcEquivalent = ispc::LinearTransferFunction_createInstance();

    // The object may not have been created.
    exitOnCondition(ispcEquivalent == NULL, "unable to create ISPC transfer function");
  }

  // A piecewise linear transfer function.
  OSP_REGISTER_TRANSFER_FUNCTION(LinearTransferFunction, piecewise_linear);

} // ::ospray

