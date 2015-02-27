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

class LightEditor : public QWidget {

Q_OBJECT

public:

  LightEditor(OSPLight light);

signals:

  void lightChanged();

protected slots:

  void alphaBetaSliderValueChanged();

protected:

  //! OSPRay light.
  OSPLight light;

  //! UI elements.
  QSlider alphaSlider;
  QSlider betaSlider;
};
