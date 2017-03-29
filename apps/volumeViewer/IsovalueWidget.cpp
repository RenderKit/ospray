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

#include "IsovalueWidget.h"
#include "IsosurfaceEditor.h"

IsovalueWidget::IsovalueWidget(IsosurfaceEditor *isosurfaceEditor) : dataRangeSet(false)
{
  QHBoxLayout * layout = new QHBoxLayout();
  layout->setSizeConstraint(QLayout::SetMinimumSize);
  setLayout(layout);

  layout->addWidget(&isovalueCheckBox);

  // Isovalue slider, defaults to median value in range.
  isovalueSlider.setValue(int(0.5f * (isovalueSlider.minimum() + isovalueSlider.maximum())));
  isovalueSlider.setOrientation(Qt::Horizontal);

  layout->addWidget(&isovalueSlider);

  // Isovalue spin box.
  isovalueSpinBox.setDecimals(6);
  isovalueSpinBox.setValue(0.5f);
  layout->addWidget(&isovalueSpinBox);

  // Connect signals and slots.
  connect(&isovalueCheckBox, SIGNAL(toggled(bool)), this, SLOT(apply()));
  connect(&isovalueSlider, SIGNAL(valueChanged(int)), this, SLOT(apply()));
  connect(&isovalueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));

  connect(this, SIGNAL(isovalueChanged()), isosurfaceEditor, SLOT(apply()));
}

void IsovalueWidget::setDataValueRange(ospcommon::vec2f dataValueRange)
{
  this->dataValueRange = dataValueRange;

  if(!dataRangeSet) {
    isovalueSpinBox.blockSignals(true);
    isovalueSpinBox.setRange(dataValueRange.x, dataValueRange.y);
    isovalueSpinBox.blockSignals(false);

    // Get isovalue based on slider position.
    float sliderPosition = float(isovalueSlider.value() - isovalueSlider.minimum()) / float(isovalueSlider.maximum() - isovalueSlider.minimum());
    float isovalue = dataValueRange.x + sliderPosition * (dataValueRange.y - dataValueRange.x);

    // Update spin box value.
    isovalueSpinBox.setValue(isovalue);

    dataRangeSet = true;
  }
  else {

    // Expand the spin box range if the range has already been set (for appropriate time series behavior).
    isovalueSpinBox.setRange(std::min((double)dataValueRange.x, isovalueSpinBox.minimum()), std::max((double)dataValueRange.y, isovalueSpinBox.maximum()));

    // Update slider position for the new range.
    float isovalue = isovalueSpinBox.value();

    float sliderPosition = float(isovalueSlider.minimum()) + (isovalue - dataValueRange.x) / (dataValueRange.y - dataValueRange.x) * float(isovalueSlider.maximum() - isovalueSlider.minimum());

    isovalueSlider.blockSignals(true);
    isovalueSlider.setValue(int(sliderPosition));
    isovalueSlider.blockSignals(false);
  }

  apply();
}

void IsovalueWidget::apply()
{
  if(sender() == &isovalueSlider) {
    float sliderPosition = float(isovalueSlider.value() - isovalueSlider.minimum()) / float(isovalueSlider.maximum() - isovalueSlider.minimum());
    float isovalue = dataValueRange.x + sliderPosition * (dataValueRange.y - dataValueRange.x);

    isovalueSpinBox.blockSignals(true);
    isovalueSpinBox.setValue(isovalue);
    isovalueSpinBox.blockSignals(false);
  }
  else if(sender() == &isovalueSpinBox) {
    float isovalue = isovalueSpinBox.value();

    float sliderPosition = float(isovalueSlider.minimum()) + (isovalue - dataValueRange.x) / (dataValueRange.y - dataValueRange.x) * float(isovalueSlider.maximum() - isovalueSlider.minimum());

    isovalueSlider.blockSignals(true);
    isovalueSlider.setValue(int(sliderPosition));
    isovalueSlider.blockSignals(false);
  }

  emit isovalueChanged();
}
