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

#include "IsosurfaceEditor.h"
#include "IsovalueWidget.h"

IsosurfaceEditor::IsosurfaceEditor()
{
  // Setup UI elements.
  layout.setSizeConstraint(QLayout::SetMinimumSize);
  layout.setAlignment(Qt::AlignTop);
  setLayout(&layout);

  QPushButton *addIsovalueButton = new QPushButton("Add isosurface");
  layout.addWidget(addIsovalueButton);
  connect(addIsovalueButton, SIGNAL(clicked()), this, SLOT(addIsovalue()));
}

void IsosurfaceEditor::setDataValueRange(ospcommon::vec2f dataValueRange)
{
  this->dataValueRange = dataValueRange;

  for (unsigned int i=0; i<isovalueWidgets.size(); i++)
    isovalueWidgets[i]->setDataValueRange(dataValueRange);
}

void IsosurfaceEditor::apply()
{
  std::vector<float> isovalues;

  for (unsigned int i=0; i<isovalueWidgets.size(); i++) {
    if (isovalueWidgets[i]->getIsovalueEnabled())
      isovalues.push_back(isovalueWidgets[i]->getIsovalue());
  }

  emit(isovaluesChanged(isovalues));
}

void IsosurfaceEditor::addIsovalue()
{
  IsovalueWidget *isovalueWidget = new IsovalueWidget(this);

  isovalueWidgets.push_back(isovalueWidget);
  layout.addWidget(isovalueWidget);

  isovalueWidget->setDataValueRange(dataValueRange);
}
