#pragma once

#include "ColorMap.h"
#include "TransferFunctionPiecewiseLinearWidget.h"
#include <QtGui>
#include <ospray/ospray.h>

class TransferFunctionEditor : public QWidget {

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

  //! Color maps.
  std::vector<ColorMap> colorMaps;

  //! OSPRay transfer function object.
  OSPTransferFunction transferFunction;

  //! Color map selection widget.
  QComboBox colorMapComboBox;

  //! Transfer function minimum data value widget.
  QDoubleSpinBox dataValueMinSpinBox;

  //! Transfer function maximum data value widget.
  QDoubleSpinBox dataValueMaxSpinBox;

  //! Transfer function widget for opacity.
  TransferFunctionPiecewiseLinearWidget transferFunctionAlphaWidget;

  //! Slider for scaling transfer function opacities.
  QSlider transferFunctionAlphaScalingSlider;
};
