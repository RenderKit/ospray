// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include <ospray/ospray.h>
#include <QtGui>

class TransferFunctionEditor : public QWidget
{
  Q_OBJECT

  public:

  //! Construct the transfer function editor with the given OSPRay transfer function object. The editor will manipulate the OSPRay object directly.
  TransferFunctionEditor(OSPTransferFunction transferFunction);

signals:

  //! Signal emitted whenever the transfer function has been changed and committed.
  void committed();

public slots:

  //! Load transfer function from file. If no filename given, a file selection dialog is created.
  void load(std::string filename = std::string());

  //! Set the data value range for the transfer function editor.
  void setDataValueRange(ospcommon::vec2f dataValueRange, bool force=false);

  //! Slot called to update transfer function opacity values based on widget values.
  void updateOpacityValues();

protected slots:

  //! Save transfer function to file.
  void save();

  //! Change active color map.
  void setColorMapIndex(int index);

  //! Slot called to update data value range based on widget values.
  void updateDataValueRange();

protected:

  //! Load default color maps.
  void loadColorMaps();

  //! OSPRay transfer function object.
  OSPTransferFunction transferFunction;

  //! Available color maps.
  std::vector<ColorMap> colorMaps;

  //! Color map selection widget.
  QComboBox colorMapComboBox;

  //! Indicates if the data value range has been set; we don't update the min / max widget values after the first time it's set.
  bool dataRangeSet;

  //! Data value range minimum widget.
  QDoubleSpinBox dataValueMinSpinBox;

  //! Data value range maximum widget.
  QDoubleSpinBox dataValueMaxSpinBox;

  //! Data value range scale (exponent, base 10) widget.
  QSpinBox dataValueScaleSpinBox;

  //! Linear transfer function widget for opacity values.
  LinearTransferFunctionWidget opacityValuesWidget;

  //! Opacity scaling slider.
  QSlider opacityScalingSlider;

};
