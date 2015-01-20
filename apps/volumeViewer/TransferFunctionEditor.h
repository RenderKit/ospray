// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ColorMap.h"
#include "LinearTransferFunctionWidget.h"
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

  void setDataValueMin(double value);
  void setDataValueMax(double value);

protected slots:

  void save();
  void setColorMapIndex(int index);

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
  LinearTransferFunctionWidget transferFunctionAlphaWidget;

  //! Slider for scaling transfer function opacities.
  QSlider transferFunctionAlphaScalingSlider;
};
