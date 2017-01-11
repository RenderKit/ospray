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

#include <ospray/ospray.h>
#include "ospcommon/box.h"
#include <QtGui>

struct SliceParameters
{
  ospcommon::vec3f origin;
  ospcommon::vec3f normal;
};

class SliceEditor;


class SliceWidget : public QFrame
{
Q_OBJECT

public:

  SliceWidget(SliceEditor *sliceEditor, ospcommon::box3f boundingBox);
  ~SliceWidget();

  SliceParameters getSliceParameters();

signals:

  void sliceChanged();
  void sliceDeleted(SliceWidget *);

public slots:

  void load(std::string filename = std::string());

protected slots:

  void apply();
  void save();
  void originSliderValueChanged(int value);
  void setAnimation(bool set);
  void animate();

protected:

  //! Bounding box of the volume.
  ospcommon::box3f boundingBox;

  //! UI elements.
  QDoubleSpinBox originXSpinBox;
  QDoubleSpinBox originYSpinBox;
  QDoubleSpinBox originZSpinBox;

  QDoubleSpinBox normalXSpinBox;
  QDoubleSpinBox normalYSpinBox;
  QDoubleSpinBox normalZSpinBox;

  QSlider originSlider;
  QPushButton originSliderAnimateButton;
  QTimer originSliderAnimationTimer;
  int originSliderAnimationDirection;

};
