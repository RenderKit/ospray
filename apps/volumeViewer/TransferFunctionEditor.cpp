/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "TransferFunctionEditor.h"

TransferFunctionEditor::TransferFunctionEditor(OSPTransferFunction transferFunction) : transferFunction(transferFunction) {

    //! Make sure we have an existing transfer function.
    if(!transferFunction)
        throw std::runtime_error("must be constructed with an existing transfer function");

    //! Load color maps.
    loadColorMaps();

    //! Setup UI elments.
    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    //! Save and load buttons.
    QWidget * saveLoadWidget = new QWidget();
    QHBoxLayout * hboxLayout = new QHBoxLayout();
    saveLoadWidget->setLayout(hboxLayout);

    QPushButton * saveButton = new QPushButton("Save");
    connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
    hboxLayout->addWidget(saveButton);

    QPushButton * loadButton = new QPushButton("Load");
    connect(loadButton, SIGNAL(clicked()), this, SLOT(load()));
    hboxLayout->addWidget(loadButton);

    layout->addWidget(saveLoadWidget);

    //! Form layout.
    QWidget * formWidget = new QWidget();
    QFormLayout * formLayout = new QFormLayout();
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget);

    //! Color map choice.
    for(unsigned int i=0; i<colorMaps.size(); i++)
        colorMapComboBox.addItem(colorMaps[i].getName().c_str());

    formLayout->addRow("Color map", &colorMapComboBox);

    //! Data value range, used as the domain for both color and opacity components of the transfer function.
    dataValueMinSpinBox.setRange(-999999., 999999.);
    dataValueMaxSpinBox.setRange(-999999., 999999.);
    dataValueMinSpinBox.setValue(0.);
    dataValueMaxSpinBox.setValue(1.);
    dataValueMinSpinBox.setDecimals(6);
    dataValueMaxSpinBox.setDecimals(6);
    formLayout->addRow("Data value min", &dataValueMinSpinBox);
    formLayout->addRow("Data value max", &dataValueMaxSpinBox);

    //! Widget containing opacity transfer function widget and scaling slider.
    QWidget * transferFunctionAlphaGroup = new QWidget();
    hboxLayout = new QHBoxLayout();
    transferFunctionAlphaGroup->setLayout(hboxLayout);

    //! Opacity transfer function widget.
    hboxLayout->addWidget(&transferFunctionAlphaWidget);

    //! Opacity scaling slider, defaults to median value in range.
    transferFunctionAlphaScalingSlider.setValue(int(0.5f * (transferFunctionAlphaScalingSlider.minimum() + transferFunctionAlphaScalingSlider.maximum())));
    transferFunctionAlphaScalingSlider.setOrientation(Qt::Vertical);
    hboxLayout->addWidget(&transferFunctionAlphaScalingSlider);

    layout->addWidget(transferFunctionAlphaGroup);

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

    connect(&colorMapComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorMapIndex(int)));
    connect(&dataValueMinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMin(double)));
    connect(&dataValueMaxSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMax(double)));
    connect(&transferFunctionAlphaWidget, SIGNAL(transferFunctionChanged()), this, SLOT(transferFunctionAlphasChanged()));
    connect(&transferFunctionAlphaScalingSlider, SIGNAL(valueChanged(int)), this, SLOT(transferFunctionAlphasChanged()));
}

void TransferFunctionEditor::transferFunctionAlphasChanged() {

    //! Default to 256 discretizations of the opacities over the domain.
    std::vector<float> transferFunctionAlphas = transferFunctionAlphaWidget.getInterpolatedValuesOverInterval(256);

    //! Alpha scaling factor (normalized in [0, 1]).
    const float alphaScalingNormalized = float(transferFunctionAlphaScalingSlider.value() - transferFunctionAlphaScalingSlider.minimum()) / float(transferFunctionAlphaScalingSlider.maximum() - transferFunctionAlphaScalingSlider.minimum());

    // alpha scaling range, for now constant at 0 -> 200.
    // this should really be bounded in [0.0, 1.0], but we allow a wider range to avoid additional parameters to the renderer / volume.
    const float alphaScalingMin = 0.f;
    const float alphaScalingMax = 200.f;

    //! Alpha scaling within the above range.
    const float alphaScaling = alphaScalingMin + alphaScalingNormalized * (alphaScalingMax - alphaScalingMin);

    //! Scale alpha values.
    for(unsigned int i=0; i<transferFunctionAlphas.size(); i++)
        transferFunctionAlphas[i] *= alphaScaling;

    OSPData transferFunctionAlphasData = ospNewData(transferFunctionAlphas.size(), OSP_FLOAT, transferFunctionAlphas.data());
    ospSetData(transferFunction, "alphas", transferFunctionAlphasData);

    //! Commit and emit signal.
    ospCommit(transferFunction);
    emit transferFunctionChanged();
}

void TransferFunctionEditor::load(std::string filename) {

    //! Get filename if not specified.
    if(filename.empty())
        filename = QFileDialog::getOpenFileName(this, tr("Load transfer function"), ".", "Transfer function files (*.tfn)").toStdString();

    if(filename.empty())
        return;

    //! Get serialized transfer function state from file.
    QFile file(filename.c_str());
    bool success = file.open(QIODevice::ReadOnly);

    if(!success)
    {
        std::cerr << "unable to open " << filename << std::endl;
        return;
    }

    QDataStream in(&file);

    int colorMapIndex;
    in >> colorMapIndex;

    double dataValueMin, dataValueMax;
    in >> dataValueMin >> dataValueMax;

    QVector<QPointF> points;
    in >> points;

    int alphaScalingIndex;
    in >> alphaScalingIndex;

    //! Update transfer function state. Update values of the UI elements directly to signal appropriate slots.
    colorMapComboBox.setCurrentIndex(colorMapIndex);
    dataValueMinSpinBox.setValue(dataValueMin);
    dataValueMaxSpinBox.setValue(dataValueMax);
    transferFunctionAlphaWidget.setPoints(points);
    transferFunctionAlphaScalingSlider.setValue(alphaScalingIndex);
}

void TransferFunctionEditor::save() {

    //! Get filename.
    QString filename = QFileDialog::getSaveFileName(this, "Save transfer function", ".", "Transfer function files (*.tfn)");

    if(filename.isNull())
        return;

    //! Make sure the filename has the proper extension.
    if(filename.endsWith(".tfn") != true)
        filename += ".tfn";

    //! Serialize transfer function state to file.
    QFile file(filename);
    bool success = file.open(QIODevice::WriteOnly);

    if(!success)
    {
        std::cerr << "unable to open " << filename.toStdString() << std::endl;
        return;
    }

    QDataStream out(&file);

    out << colorMapComboBox.currentIndex();
    out << dataValueMinSpinBox.value();
    out << dataValueMaxSpinBox.value();
    out << transferFunctionAlphaWidget.getPoints();
    out << transferFunctionAlphaScalingSlider.value();
}

void TransferFunctionEditor::setColorMapIndex(int index) {

    //! Set transfer function color properties for this color map.
    std::vector<osp::vec3f> colors = colorMaps[index].getColors();

    OSPData transferFunctionColorsData = ospNewData(colors.size(), OSP_FLOAT3, colors.data());
    ospSetData(transferFunction, "colors", transferFunctionColorsData);

    //! Set transfer function widget background image.
    transferFunctionAlphaWidget.setBackgroundImage(colorMaps[index].getImage());

    //! Commit and emit signal.
    ospCommit(transferFunction);
    emit transferFunctionChanged();
}

void TransferFunctionEditor::setDataValueMin(double value) {

    //! Set as the minimum value in the domain for both color and opacity components of the transfer function.
    ospSetf(transferFunction, "colorValueMin", float(value));
    ospSetf(transferFunction, "alphaValueMin", float(value));

    //! Commit and emit signal.
    ospCommit(transferFunction);
    emit transferFunctionChanged();
}

void TransferFunctionEditor::setDataValueMax(double value) {

    //! Set as the maximum value in the domain for both color and opacity components of the transfer function.
    ospSetf(transferFunction, "colorValueMax", float(value));
    ospSetf(transferFunction, "alphaValueMax", float(value));

    //! Commit and emit signal.
    ospCommit(transferFunction);
    emit transferFunctionChanged();
}

void TransferFunctionEditor::loadColorMaps() {

    //! Color maps based on ParaView default color maps.

    std::vector<osp::vec3f> colors;

    //! Jet.
    colors.clear();
    colors.push_back(osp::vec3f(0         , 0           , 0.562493   ));
    colors.push_back(osp::vec3f(0         , 0           , 1          ));
    colors.push_back(osp::vec3f(0         , 1           , 1          ));
    colors.push_back(osp::vec3f(0.500008  , 1           , 0.500008   ));
    colors.push_back(osp::vec3f(1         , 1           , 0          ));
    colors.push_back(osp::vec3f(1         , 0           , 0          ));
    colors.push_back(osp::vec3f(0.500008  , 0           , 0          ));
    colorMaps.push_back(ColorMap("Jet", colors));

    //! Ice / fire.
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
    colorMaps.push_back(ColorMap("Ice / Fire", colors));

    //! Cool to warm.
    colors.clear();
    colors.push_back(osp::vec3f(0.231373  , 0.298039    , 0.752941    ));
    colors.push_back(osp::vec3f(0.865003  , 0.865003    , 0.865003    ));
    colors.push_back(osp::vec3f(0.705882  , 0.0156863   , 0.14902     ));
    colorMaps.push_back(ColorMap("Cool to Warm", colors));

    //! Blue to red rainbow.
    colors.clear();
    colors.push_back(osp::vec3f(0         , 0           , 1           ));
    colors.push_back(osp::vec3f(1         , 0           , 0           ));
    colorMaps.push_back(ColorMap("Blue to Red Rainbow", colors));

    //! Grayscale.
    colors.clear();
    colors.push_back(osp::vec3f(0.));
    colors.push_back(osp::vec3f(1.));
    colorMaps.push_back(ColorMap("Grayscale", colors));
}
