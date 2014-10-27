/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/Data.h"
#include "ospray/common/OspCommon.h"
#include "ospray/transferfunction/LinearTransferFunction.h"
#include "TransferFunction_ispc.h"

namespace ospray {

  void LinearTransferFunction::commit() 
  {
    // Create the equivalent ISPC transfer function.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    // Retrieve the color and opacity values.
    Data *colors = getParamData("colors", NULL);  Data *alphas = getParamData("alphas", NULL);

    // Set the color values.
    if (colors) ispc::LinearTransferFunction_setColorValues(ispcEquivalent, colors->numItems, (ispc::vec3f *) colors->data);

    // Set the color value range.
    if (colors) ispc::LinearTransferFunction_setColorRange(ispcEquivalent, getParamf("colorValueMin", 0.0f), getParamf("colorValueMax", 1.0f));

    // Set the opacity values.
    if (alphas) ispc::LinearTransferFunction_setAlphaValues(ispcEquivalent, alphas->numItems, (float *) alphas->data);

    // Set the opacity value range.
    if (alphas) ispc::LinearTransferFunction_setAlphaRange(ispcEquivalent, getParamf("alphaValueMin", 0.0f), getParamf("alphaValueMax", 1.0f));
	
    // Set the value range that the transfer function covers
    vec2f range = getParam2f("range", vec2f(0.f,1.f));
    ispc::TransferFunction_setValueRange(ispcEquivalent, range.x, range.y);
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

} // namespace ospray

