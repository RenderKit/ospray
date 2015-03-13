// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include <QtGui>
#include <ospray/ospray.h>

class SliceWidget : public QFrame
{
Q_OBJECT

public:

  SliceWidget(std::vector<OSPModel> models, osp::box3f volumeBounds);
  ~SliceWidget();

signals:

  void sliceChanged();

public slots:

  void autoApply();
  void load(std::string filename = std::string());

protected slots:

  void apply();
  void save();
  void originSliderValueChanged(int value);
  void setAnimation(bool set);
  void animate();

protected:

  //! OSPRay models.
  std::vector<OSPModel> models;

  //! Bounding box of the volume.
  osp::box3f volumeBounds;

  //! OSPRay triangle mesh for the slice.
  OSPTriangleMesh triangleMesh;

  //! UI elements.
  QCheckBox autoApplyCheckBox;

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
