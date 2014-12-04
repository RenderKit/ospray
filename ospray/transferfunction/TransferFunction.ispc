/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/transferfunction/TransferFunction.ih"

export void TransferFunction_setValueRange(void *uniform pointer, const uniform vec2f &value) {

    //! Cast to the actual type.
    TransferFunction *uniform transferFunction = (TransferFunction *uniform) pointer;

    //! Set the transfer function value range.
    transferFunction->valueRange = value;

}

