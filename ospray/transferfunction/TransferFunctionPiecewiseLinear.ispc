//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "TransferFunction.ih"

struct TransferFunctionPiecewiseLinear
{
    TransferFunction inherited;

    uniform int numColors;
    uniform vec3f * uniform colors;
    uniform float colorValueMin;
    uniform float colorValueMax;

    uniform int numAlphas;
    uniform float * uniform alphas;
    uniform float alphaValueMin;
    uniform float alphaValueMax;
};

inline varying vec3f TransferFunctionPiecewiseLinear_lookupColor(uniform TransferFunctionPtr _transferFunction, const varying float &value)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear *uniform)_transferFunction;

    if(transferFunction->numColors == 0)
        return make_vec3f(1.);

    if(value <= transferFunction->colorValueMin)
        return transferFunction->colors[0];
    else if(value >= transferFunction->colorValueMax)
        return transferFunction->colors[transferFunction->numColors - 1];
    else
    {
        float index = (value - transferFunction->colorValueMin) / (transferFunction->colorValueMax - transferFunction->colorValueMin) * (float)(transferFunction->numColors - 1);

        return transferFunction->colors[(int)index] + (index - floor(index)) * transferFunction->colors[(int)(index + 1)];
    }
}

inline varying float TransferFunctionPiecewiseLinear_lookupAlpha(uniform TransferFunctionPtr _transferFunction, const varying float &value)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear *uniform)_transferFunction;

    if(transferFunction->numAlphas == 0)
        return 1.;

    if(value <= transferFunction->alphaValueMin)
        return transferFunction->alphas[0];
    else if(value >= transferFunction->alphaValueMax)
        return transferFunction->alphas[transferFunction->numAlphas - 1];
    else
    {
        float index = (value - transferFunction->alphaValueMin) / (transferFunction->alphaValueMax - transferFunction->alphaValueMin) * (float)(transferFunction->numAlphas - 1);

        return transferFunction->alphas[(int)index] + (index - floor(index)) * transferFunction->alphas[(int)(index + 1)];
    }
}

uniform TransferFunctionPtr TransferFunctionPiecewiseLinear_create()
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = uniform new uniform TransferFunctionPiecewiseLinear;

    transferFunction->inherited.lookupColor = TransferFunctionPiecewiseLinear_lookupColor;
    transferFunction->inherited.lookupAlpha = TransferFunctionPiecewiseLinear_lookupAlpha;

    transferFunction->numColors = 0;
    transferFunction->colors = NULL;
    transferFunction->colorValueMin = 0.;
    transferFunction->colorValueMax = 1.;

    transferFunction->numAlphas = 0;
    transferFunction->alphas = NULL;
    transferFunction->alphaValueMin = 0.;
    transferFunction->alphaValueMax = 1.;

    return (uniform TransferFunctionPtr)transferFunction;
}

export uniform ExtTransferFunctionPtr _TransferFunctionPiecewiseLinear_create()
{
    return (ExtTransferFunctionPtr)TransferFunctionPiecewiseLinear_create();
}

export void _TransferFunctionPiecewiseLinear_destroy(uniform ExtTransferFunctionPtr _transferFunction)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear * uniform)_transferFunction;

    if(transferFunction->colors != NULL)
    {
        delete[] transferFunction->colors;
    }

    if(transferFunction->alphas != NULL)
    {
        delete[] transferFunction->alphas;
    }

    delete transferFunction;
}

export void _TransferFunctionPiecewiseLinear_setColors(uniform ExtTransferFunctionPtr _transferFunction, const uniform size_t &numColors, uniform vec3f * uniform colors)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear * uniform)_transferFunction;

    transferFunction->numColors = numColors;

    if(transferFunction->colors != NULL)
    {
        delete[] transferFunction->colors;
    }

    transferFunction->colors = uniform new uniform vec3f[numColors];
    memcpy(transferFunction->colors, colors, numColors*sizeof(uniform vec3f));
}

export void _TransferFunctionPiecewiseLinear_setColorValueRange(uniform ExtTransferFunctionPtr _transferFunction, const uniform float &valueMin, const uniform float &valueMax)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear * uniform)_transferFunction;

    transferFunction->colorValueMin = valueMin;
    transferFunction->colorValueMax = valueMax;
}

export void _TransferFunctionPiecewiseLinear_setAlphas(uniform ExtTransferFunctionPtr _transferFunction, const uniform size_t &numAlphas, uniform float * uniform alphas)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear * uniform)_transferFunction;

    transferFunction->numAlphas = numAlphas;

    if(transferFunction->alphas != NULL)
    {
        delete[] transferFunction->alphas;
    }

    transferFunction->alphas = uniform new uniform float[numAlphas];
    memcpy(transferFunction->alphas, alphas, numAlphas*sizeof(uniform float));
}

export void _TransferFunctionPiecewiseLinear_setAlphaValueRange(uniform ExtTransferFunctionPtr _transferFunction, const uniform float &valueMin, const uniform float &valueMax)
{
    uniform TransferFunctionPiecewiseLinear * uniform transferFunction = (uniform TransferFunctionPiecewiseLinear * uniform)_transferFunction;

    transferFunction->alphaValueMin = valueMin;
    transferFunction->alphaValueMax = valueMax;
}
