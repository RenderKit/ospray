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

#include "SliceEditor.h"
#include <algorithm>

SliceEditor::SliceEditor(ospcommon::box3f boundingBox) 
  : boundingBox(boundingBox)
{
  // Setup UI elements.
  layout.setSizeConstraint(QLayout::SetMinimumSize);
  layout.setAlignment(Qt::AlignTop);
  setLayout(&layout);

  QPushButton *addSliceButton = new QPushButton("Add slice");
  layout.addWidget(addSliceButton);
  connect(addSliceButton, SIGNAL(clicked()), this, SLOT(addSlice()));
}

void SliceEditor::addSlice(std::string filename)
{
  SliceWidget *sliceWidget = new SliceWidget(this, boundingBox);

  sliceWidgets.push_back(sliceWidget);
  layout.addWidget(sliceWidget);

  if (!filename.empty())
    sliceWidget->load(filename);
}

void SliceEditor::apply()
{
  std::vector<SliceParameters> sliceParameters;

  for (unsigned int i=0; i<sliceWidgets.size(); i++) {
    sliceParameters.push_back(sliceWidgets[i]->getSliceParameters());
  }

  emit(slicesChanged(sliceParameters));
}

void SliceEditor::deleteSlice(SliceWidget *sliceWidget)
{
  // This is triggered from the SliceWidget destructor, so no need to delete...
  std::vector<SliceWidget *>::iterator position = std::find(sliceWidgets.begin(), sliceWidgets.end(), sliceWidget);

  if (position != sliceWidgets.end())
    sliceWidgets.erase(position);

  apply();
}
