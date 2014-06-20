//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "TransferFunctionPiecewiseLinear.h"
#include "TransferFunctionPiecewiseLinear_ispc.h"
#include "ospray/common/ospcommon.h"
#include "ospray/common/data.h"

namespace ospray
{
    vec3f colorNone = vec3f(1.);
    float alphaNone = 1.;

    TransferFunctionPiecewiseLinear::TransferFunctionPiecewiseLinear()
    {
        // defaults
        colors_.push_back(vec3f(0.));
        colors_.push_back(vec3f(1.));
        colorValueMin_ = 0.;
        colorValueMax_ = 1.;

        alphas_.push_back(0.);
        alphas_.push_back(1.);
        alphaValueMin_ = 0.;
        alphaValueMax_ = 1.;
    }

    void TransferFunctionPiecewiseLinear::commit()
    {
        if(ispcEquivalent == NULL)
            ispcEquivalent = ispc::_TransferFunctionPiecewiseLinear_create();

        // colors
        Data * colorsData = getParamData("colors", NULL);

        if(colorsData != NULL)
        {
            vec3f * colors = (vec3f *)colorsData->data;
            size_t numColors = colorsData->numItems;

            std::vector<vec3f> colorsVector(&colors[0], &colors[numColors]);

            setColors(colorsVector);
        }
        else
        {
            // defaults
            setColors(colors_);
        }

        float colorValueMin = getParamf("colorValueMin", colorValueMin_);
        float colorValueMax = getParamf("colorValueMax", colorValueMax_);

        setColorValueRange(colorValueMin, colorValueMax);

        // alphas
        Data * alphasData = getParamData("alphas", NULL);

        if(alphasData != NULL)
        {
            float * alphas = (float *)alphasData->data;
            size_t numAlphas = alphasData->numItems;

            std::vector<float> alphasVector(&alphas[0], &alphas[numAlphas]);

            setAlphas(alphasVector);
        }
        else
        {
            // defaults
            setAlphas(alphas_);
        }

        float alphaValueMin = getParamf("alphaValueMin", alphaValueMin_);
        float alphaValueMax = getParamf("alphaValueMax", alphaValueMax_);

        setAlphaValueRange(alphaValueMin, alphaValueMax);
    }

    ispc::_TransferFunction * TransferFunctionPiecewiseLinear::createIE()
    {
        assert(ispcEquivalent == NULL);
        this->ispcEquivalent = ispc::_TransferFunctionPiecewiseLinear_create();
        return (ispc::_TransferFunction *)this->ispcEquivalent;
    }

    vec3f TransferFunctionPiecewiseLinear::lookupColor(float value)
    {
        if(colors_.size() == 0)
            return colorNone;

        if(value <= colorValueMin_)
            return colors_[0];
        else if(value >= colorValueMax_)
            return colors_[colors_.size()-1];
        else
        {
            float index = (value - colorValueMin_) / (colorValueMax_ - colorValueMin_) * float(colors_.size() - 1);

            return colors_[int(index)] + (index - floorf(index)) * colors_[int(index + 1)];
        }
    }

    float TransferFunctionPiecewiseLinear::lookupAlpha(float value)
    {
        if(alphas_.size() == 0)
            return alphaNone;

        if(value <= alphaValueMin_)
            return alphas_[0];
        else if(value >= alphaValueMax_)
            return alphas_[alphas_.size()-1];
        else
        {
            float index = (value - alphaValueMin_) / (alphaValueMax_ - alphaValueMin_) * float(alphas_.size() - 1);

            return alphas_[int(index)] + (index - floorf(index)) * alphas_[int(index + 1)];
        }
    }

    void TransferFunctionPiecewiseLinear::setColors(std::vector<vec3f> colors)
    {
        colors_ = colors;
        ispc::_TransferFunctionPiecewiseLinear_setColors((ispc::_TransferFunction*)getIE(), colors.size(), (ispc::vec3f *)colors.data());
    }

    void TransferFunctionPiecewiseLinear::setColorValueRange(float valueMin, float valueMax)
    {
        colorValueMin_ = valueMin;
        colorValueMax_ = valueMax;
        ispc::_TransferFunctionPiecewiseLinear_setColorValueRange((ispc::_TransferFunction*)getIE(), valueMin, valueMax);
    }

    void TransferFunctionPiecewiseLinear::setAlphas(std::vector<float> alphas)
    {
        alphas_ = alphas;
        ispc::_TransferFunctionPiecewiseLinear_setAlphas((ispc::_TransferFunction*)getIE(), alphas.size(), (float *)alphas.data());
    }

    void TransferFunctionPiecewiseLinear::setAlphaValueRange(float valueMin, float valueMax)
    {
        alphaValueMin_ = valueMin;
        alphaValueMax_ = valueMax;
        ispc::_TransferFunctionPiecewiseLinear_setAlphaValueRange((ispc::_TransferFunction*)getIE(), valueMin, valueMax);
    }

    OSP_REGISTER_TRANSFER_FUNCTION(TransferFunctionPiecewiseLinear, TransferFunctionPiecewiseLinear);

} // namespace ospray
