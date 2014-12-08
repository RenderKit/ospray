/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/Data.h"
#include "ospray/common/OSPCommon.h"
#include "ospray/transferfunction/LinearTransferFunction.h"
#include "TransferFunction_ispc.h"

namespace ospray {

    void LinearTransferFunction::commit() {

        // Create the equivalent ISPC transfer function.
        if (ispcEquivalent == NULL) createEquivalentISPC();

        // Retrieve the color and opacity values.
        colorValues = getParamData("colors", NULL);  opacityValues = getParamData("opacities", NULL);

        // Set the color values.
        if (colorValues) ispc::LinearTransferFunction_setColorValues(ispcEquivalent, colorValues->numItems, (ispc::vec3f *) colorValues->data);

        // Set the opacity values.
        if (opacityValues) ispc::LinearTransferFunction_setOpacityValues(ispcEquivalent, opacityValues->numItems, (float *) opacityValues->data);

        // Set the value range that the transfer function covers.
        vec2f valueRange = getParam2f("valueRange", vec2f(0.0f, 1.0f));  ispc::TransferFunction_setValueRange(ispcEquivalent, (const ispc::vec2f &) valueRange);

        // Notify listeners that the transfer function has changed.
        notifyListenersThatObjectGotChanged();

    }

    void LinearTransferFunction::createEquivalentISPC() {

        // The equivalent ISPC transfer function must not exist yet.
        exitOnCondition(ispcEquivalent != NULL, "attempt to overwrite an existing ISPC transfer function");

        // Create the equivalent ISPC transfer function.
        ispcEquivalent = ispc::LinearTransferFunction_createInstance();

        // The object may not have been created.
        exitOnCondition(ispcEquivalent == NULL, "unable to create ISPC transfer function");

    }

} // namespace ospray

