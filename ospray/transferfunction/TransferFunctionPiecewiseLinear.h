//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#pragma once

#include "TransferFunction.h"

namespace ospray
{
    //! \brief A concrete implementation of the transfer function class
    //!  for piecewise linear transfer functions.
    //!
    class TransferFunctionPiecewiseLinear : public TransferFunction
    {
    public:

        //! Constructor.
        TransferFunctionPiecewiseLinear();

        //! A string description of this class.
        virtual std::string toString() const { return "ospray::TransferFunctionPiecewiseLinear"; }

        //! Allocate storage and populate the transfer function.
        virtual void commit();

        //! Create the equivalent ISPC transfer function.
        virtual ispc::_TransferFunction * createIE();

        //! Look up the color corresponding to the provided value.
        virtual vec3f lookupColor(float value);

        //! Look up the opacity corresponding to the provided value.
        virtual float lookupAlpha(float value);

    protected:

        //! Internal method for setting colors of the transfer function.
        void setColors(std::vector<vec3f> colors);

        //! Internal method for setting the value range associated with the color
        //! component of the transfer function.
        void setColorValueRange(float valueMin, float valueMax);

        //! Internal method for setting the opacity values of the transfer function.
        void setAlphas(std::vector<float> alphas);

        //! Internal method for setting the value range associated with the opacity
        //! component of the transfer function.
        void setAlphaValueRange(float valueMin, float valueMax);

        std::vector<vec3f> colors_;
        float colorValueMin_;
        float colorValueMax_;

        std::vector<float> alphas_;
        float alphaValueMin_;
        float alphaValueMax_;
    };

} // namespace ospray
