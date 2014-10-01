/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "TransferFunctionEditor.h"

TransferFunctionEditor::TransferFunctionEditor(OSPTransferFunction transferFunction)
{
    // assign transfer function
    if(!transferFunction)
        throw std::runtime_error("must be constructed with an existing transfer function");

    transferFunction_ = transferFunction;

    // load color maps
    loadColorMaps();

    // setup UI elments
    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    // form layout
    QWidget * formWidget = new QWidget();
    QFormLayout * formLayout = new QFormLayout();
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget);

    // color map choice
    QComboBox * colorMapComboBox = new QComboBox();

    for(unsigned int i=0; i<colorMaps_.size(); i++)
    {
        colorMapComboBox->addItem(colorMaps_[i].getName().c_str());
    }

    formLayout->addRow("Color map", colorMapComboBox);

    // data value range, used as the domain for both color and opacity components of the transfer function
    QDoubleSpinBox * dataValueMinSpinBox = new QDoubleSpinBox();
    QDoubleSpinBox * dataValueMaxSpinBox = new QDoubleSpinBox();
    dataValueMinSpinBox->setRange(-999999., 999999.);
    dataValueMaxSpinBox->setRange(-999999., 999999.);
    dataValueMinSpinBox->setValue(0.);
    dataValueMaxSpinBox->setValue(1.);
    dataValueMinSpinBox->setDecimals(6);
    dataValueMaxSpinBox->setDecimals(6);
    formLayout->addRow("Data value min", dataValueMinSpinBox);
    formLayout->addRow("Data value max", dataValueMaxSpinBox);

    // opacity transfer function widget
    layout->addWidget(&transferFunctionAlphaWidget_);

    //! The Qt 4.8 documentation says: "by default, for every connection you
    //! make, a signal is emitted".  But this isn't happening here (Qt 4.8.5,
    //! Mac OS 10.9.4) so we manually invoke these functions so the transfer
    //! function is fully populated before the first render call.
    //!
    //! Unfortunately, each invocation causes all transfer function fields to
    //! be rewritten on the ISPC side (due to repeated recommits of the OSPRay
    //! transfer function object).
    //!
    setColorMapIndex(0);
    transferFunctionAlphasChanged();
    setDataValueMin(0.0);
    setDataValueMax(1.0);

    connect(colorMapComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorMapIndex(int)));
    connect(dataValueMinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMin(double)));
    connect(dataValueMaxSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMax(double)));
    connect(&transferFunctionAlphaWidget_, SIGNAL(transferFunctionChanged()), this, SLOT(transferFunctionAlphasChanged()));

}

void TransferFunctionEditor::transferFunctionAlphasChanged()
{

    // default to 256 discretizations of the opacities over the domain
    std::vector<float> transferFunctionAlphas = transferFunctionAlphaWidget_.getInterpolatedValuesOverInterval(256);

    OSPData transferFunctionAlphasData = ospNewData(transferFunctionAlphas.size(), OSP_FLOAT, transferFunctionAlphas.data());
    ospSetData(transferFunction_, "alphas", transferFunctionAlphasData);

    // commit and emit signal
    ospCommit(transferFunction_);
    emit transferFunctionChanged();

}

void TransferFunctionEditor::setColorMapIndex(int index)
{

    // set transfer function color properties for this color map
    std::vector<osp::vec3f> colors = colorMaps_[index].getColors();

    OSPData transferFunctionColorsData = ospNewData(colors.size(), OSP_FLOAT3, colors.data());
    ospSetData(transferFunction_, "colors", transferFunctionColorsData);

    // set transfer function widget background image
    transferFunctionAlphaWidget_.setBackgroundImage(colorMaps_[index].getImage());

    // commit and emit signal
    ospCommit(transferFunction_);
    emit transferFunctionChanged();

}

void TransferFunctionEditor::setDataValueMin(double value)
{

    // set as the minimum value in the domain for both color and opacity components of the transfer function
    ospSetf(transferFunction_, "colorValueMin", float(value));
    ospSetf(transferFunction_, "alphaValueMin", float(value));

    // commit and emit signal
    ospCommit(transferFunction_);
    emit transferFunctionChanged();

}

void TransferFunctionEditor::setDataValueMax(double value)
{

    // set as the maximum value in the domain for both color and opacity components of the transfer function
    ospSetf(transferFunction_, "colorValueMax", float(value));
    ospSetf(transferFunction_, "alphaValueMax", float(value));

    // commit and emit signal
    ospCommit(transferFunction_);
    emit transferFunctionChanged();

}

void TransferFunctionEditor::loadColorMaps()
{
    // color maps based on ParaView default color maps

    std::vector<osp::vec3f> colors;

    // jet
    colors.clear();
    colors.push_back(osp::vec3f(0         , 0           , 0.562493   ));
    colors.push_back(osp::vec3f(0         , 0           , 1          ));
    colors.push_back(osp::vec3f(0         , 1           , 1          ));
    colors.push_back(osp::vec3f(0.500008  , 1           , 0.500008   ));
    colors.push_back(osp::vec3f(1         , 1           , 0          ));
    colors.push_back(osp::vec3f(1         , 0           , 0          ));
    colors.push_back(osp::vec3f(0.500008  , 0           , 0          ));
    colorMaps_.push_back(ColorMap("Jet", colors));

    // ice / fire
    colors.clear();
    colors.push_back(osp::vec3f(0         , 0           , 0           ));
    colors.push_back(osp::vec3f(0         , 0.120394    , 0.302678    ));
    colors.push_back(osp::vec3f(0         , 0.216587    , 0.524575    ));
    colors.push_back(osp::vec3f(0.0552529 , 0.345022    , 0.659495    ));
    colors.push_back(osp::vec3f(0.128054  , 0.492592    , 0.720287    ));
    colors.push_back(osp::vec3f(0.188952  , 0.641306    , 0.792096    ));
    colors.push_back(osp::vec3f(0.327672  , 0.784939    , 0.873426    ));
    colors.push_back(osp::vec3f(0.60824   , 0.892164    , 0.935546    ));
    colors.push_back(osp::vec3f(0.881376  , 0.912184    , 0.818097    ));
    colors.push_back(osp::vec3f(0.9514    , 0.835615    , 0.449271    ));
    colors.push_back(osp::vec3f(0.904479  , 0.690486    , 0           ));
    colors.push_back(osp::vec3f(0.854063  , 0.510857    , 0           ));
    colors.push_back(osp::vec3f(0.777096  , 0.330175    , 0.000885023 ));
    colors.push_back(osp::vec3f(0.672862  , 0.139086    , 0.00270085  ));
    colors.push_back(osp::vec3f(0.508812  , 0           , 0           ));
    colors.push_back(osp::vec3f(0.299413  , 0.000366217 , 0.000549325 ));
    colors.push_back(osp::vec3f(0.0157473 , 0.00332647  , 0           ));
    colorMaps_.push_back(ColorMap("Ice / Fire", colors));

    // cool to warm
    colors.clear();
    colors.push_back(osp::vec3f(0.231373  , 0.298039    , 0.752941    ));
    colors.push_back(osp::vec3f(0.865003  , 0.865003    , 0.865003    ));
    colors.push_back(osp::vec3f(0.705882  , 0.0156863   , 0.14902     ));
    colorMaps_.push_back(ColorMap("Cool to Warm", colors));

    // blue to red rainbow
    colors.clear();
    colors.push_back(osp::vec3f(0         , 0           , 1           ));
    colors.push_back(osp::vec3f(1         , 0           , 0           ));
    colorMaps_.push_back(ColorMap("Blue to Red Rainbow", colors));

    // grayscale
    colors.clear();
    colors.push_back(osp::vec3f(0.));
    colors.push_back(osp::vec3f(1.));
    colorMaps_.push_back(ColorMap("Grayscale", colors));
}
