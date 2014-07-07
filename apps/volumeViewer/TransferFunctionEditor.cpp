#include "TransferFunctionEditor.h"

TransferFunctionEditor::TransferFunctionEditor(OSPTransferFunction transferFunction)
{
    assert(transferFunction);
    transferFunction_ = transferFunction;

    // load color maps
    loadColorMaps();

    // set first available color map
    if(colorMaps_.size() > 0)
    {
        setColorMapIndex(0);
    }

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

    connect(colorMapComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorMapIndex(int)));

    // data value range, used as the domain for both color and opacity components of the transfer function
    QDoubleSpinBox * dataValueMinSpinBox = new QDoubleSpinBox();
    QDoubleSpinBox * dataValueMaxSpinBox = new QDoubleSpinBox();
    dataValueMinSpinBox->setRange(-999999., 999999.);
    dataValueMaxSpinBox->setRange(-999999., 999999.);
    dataValueMinSpinBox->setValue(0.);
    dataValueMaxSpinBox->setValue(1.);
    formLayout->addRow("Data value min", dataValueMinSpinBox);
    formLayout->addRow("Data value max", dataValueMaxSpinBox);

    connect(dataValueMinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMin(double)));
    connect(dataValueMaxSpinBox, SIGNAL(valueChanged(double)), this, SLOT(setDataValueMax(double)));

    // opacity transfer function widget
    layout->addWidget(&transferFunctionAlphaWidget_);

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

    OSPData transferFunctionColorsData = ospNewData(colors.size(), OSP_vec3f, colors.data());
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
    // default color map
    std::vector<osp::vec3f> defaultColors;
    defaultColors.push_back(osp::vec3f(0         , 1           , 0           ));
    defaultColors.push_back(osp::vec3f(0         , 0.120394    , 0.302678    ));
    defaultColors.push_back(osp::vec3f(0         , 0.216587    , 0.524575    ));
    defaultColors.push_back(osp::vec3f(0.0552529 , 0.345022    , 0.659495    ));
    defaultColors.push_back(osp::vec3f(0.128054  , 0.492592    , 0.720287    ));
    defaultColors.push_back(osp::vec3f(0.188952  , 0.641306    , 0.792096    ));
    defaultColors.push_back(osp::vec3f(0.327672  , 0.784939    , 0.873426    ));
    defaultColors.push_back(osp::vec3f(0.60824   , 0.892164    , 0.935546    ));
    defaultColors.push_back(osp::vec3f(0.881376  , 0.912184    , 0.818097    ));
    defaultColors.push_back(osp::vec3f(0.9514    , 0.835615    , 0.449271    ));
    defaultColors.push_back(osp::vec3f(0.904479  , 0.690486    , 0           ));
    defaultColors.push_back(osp::vec3f(0.854063  , 0.510857    , 0           ));
    defaultColors.push_back(osp::vec3f(0.777096  , 0.330175    , 0.000885023 ));
    defaultColors.push_back(osp::vec3f(0.672862  , 0.139086    , 0.00270085  ));
    defaultColors.push_back(osp::vec3f(0.508812  , 0           , 0           ));
    defaultColors.push_back(osp::vec3f(0.299413  , 0.000366217 , 0.000549325 ));
    // defaultColors.push_back(osp::vec3f(0.0157473 , 0.00332647  , 0           ));
    defaultColors.push_back(osp::vec3f(0.0157473 , 0.00332647  , 1           ));

    colorMaps_.push_back(ColorMap("default", defaultColors));

    // monochrome
    std::vector<osp::vec3f> monochromeColors;
    monochromeColors.push_back(osp::vec3f(0.));
    monochromeColors.push_back(osp::vec3f(1.));

    colorMaps_.push_back(ColorMap("monochrome", monochromeColors));
}
