//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/common/data.h"
#include "ospray/common/ospcommon.h"
#include "ospray/transferfunction/LinearTransferFunction.h"
#include "TransferFunction_ispc.h"

namespace ospray {

    void LinearTransferFunction::commit() {

        //! Create the equivalent ISPC transfer function.
        if (getEquivalentISPC() == NULL) createEquivalentISPC();

        //! Retrieve the color and opacity values.
        Data *colors = getParamData("colors", NULL);  Data *alphas = getParamData("alphas", NULL);

        //! Set the color values.
        if (colors) ispc::LinearTransferFunction_setColorValues(getEquivalentISPC(), colors->numItems, (ispc::vec3f *) colors->data);

        //! Set the color value range.
        if (colors) ispc::LinearTransferFunction_setColorRange(getEquivalentISPC(), getParamf("colorValueMin", 0.0f), getParamf("colorValueMax", 1.0f));

        //! Set the opacity values.
        if (alphas) ispc::LinearTransferFunction_setAlphaValues(getEquivalentISPC(), alphas->numItems, (float *) alphas->data);

        //! Set the opacity value range.
        if (alphas) ispc::LinearTransferFunction_setAlphaRange(getEquivalentISPC(), getParamf("alphaValueMin", 0.0f), getParamf("alphaValueMax", 1.0f));
	
	if (alphas) ispc::TransferFunction_PrecomputeMinMaxValues(getEquivalentISPC());

    }

    void LinearTransferFunction::createEquivalentISPC() {

        //! The equivalent ISPC transfer function must not exist yet.
        exitOnCondition(ispcEquivalent != NULL, "attempt to overwrite an existing ISPC transfer function");

        //! Create the equivalent ISPC transfer function.
        ispcEquivalent = ispc::LinearTransferFunction_createInstance();

        //! The object may not have been created.
        exitOnCondition(ispcEquivalent == NULL, "unable to create ISPC transfer function");

    }

} // namespace ospray

