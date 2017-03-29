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
#include "ospcommon/vec.h"
#include <QtGui>
#include <vector>

class IsovalueWidget;

class IsosurfaceEditor : public QWidget
{
Q_OBJECT

public:

  IsosurfaceEditor();

signals:

  void isovaluesChanged(std::vector<float> values);

public slots:

  void setDataValueRange(ospcommon::vec2f dataValueRange);

  void apply();

protected slots:

  void addIsovalue();

protected:

  ospcommon::vec2f dataValueRange;

  QVBoxLayout layout;

  std::vector<IsovalueWidget *> isovalueWidgets;

};
