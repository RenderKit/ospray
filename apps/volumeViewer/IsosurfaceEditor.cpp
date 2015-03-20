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

#include "IsosurfaceEditor.h"

IsosurfaceEditor::IsosurfaceEditor()
{
  //! Setup UI elements.
  QHBoxLayout * layout = new QHBoxLayout();
  layout->setSizeConstraint(QLayout::SetMinimumSize);
  setLayout(layout);

  layout->addWidget(&isovalueCheckBox);

  //! Isovalue slider, defaults to median value in range.
  isovalueSlider.setValue(int(0.5f * (isovalueSlider.minimum() + isovalueSlider.maximum())));
  isovalueSlider.setOrientation(Qt::Horizontal);

  layout->addWidget(&isovalueSlider);

  //! Connect signals and slots.
  connect(&isovalueCheckBox, SIGNAL(toggled(bool)), this, SLOT(apply()));
  connect(&isovalueSlider, SIGNAL(valueChanged(int)), this, SLOT(apply()));

  //! Apply with default values.
  apply();
}

void IsosurfaceEditor::apply()
{
  std::vector<float> isovalues;

  if(isovalueCheckBox.isChecked()) {
    float sliderPosition = float(isovalueSlider.value()) / float(isovalueSlider.maximum() - isovalueSlider.minimum());
    isovalues.push_back(dataValueRange.x + sliderPosition * (dataValueRange.y - dataValueRange.x));
  }

  emit(isovaluesChanged(isovalues));
}
