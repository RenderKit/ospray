/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ColorMap.h"
#include "TransferFunctionPiecewiseLinearWidget.h"
#include <QtGui>
#include <ospray/ospray.h>

class TransferFunctionEditor : public QWidget
{
Q_OBJECT

public:

    TransferFunctionEditor(OSPTransferFunction transferFunction);

signals:

    void transferFunctionChanged();

public slots:

    void transferFunctionAlphasChanged();

    void load(std::string filename = std::string());

protected slots:

    void save();
    void setColorMapIndex(int index);
    void setDataValueMin(double value);
    void setDataValueMax(double value);

protected:

    void loadColorMaps();

    // color maps
    std::vector<ColorMap> colorMaps_;

    // OSPRay transfer function object
    OSPTransferFunction transferFunction_;

    // transfer function widget for opacity
    TransferFunctionPiecewiseLinearWidget transferFunctionAlphaWidget_;

    //! UI elements.
    QComboBox colorMapComboBox_;
    QDoubleSpinBox dataValueMinSpinBox_;
    QDoubleSpinBox dataValueMaxSpinBox_;
};
