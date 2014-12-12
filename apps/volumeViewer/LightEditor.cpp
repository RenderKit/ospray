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

#include "LightEditor.h"

LightEditor::LightEditor(OSPLight light) : light(light) {

  //! Make sure we have an existing transfer light.
  if (!light) throw std::runtime_error("LightEditor: must be constructed with an existing light");

  //! Setup UI elments.
  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);

  //! Form layout.
  QWidget * formWidget = new QWidget();
  QFormLayout * formLayout = new QFormLayout();
  formWidget->setLayout(formLayout);
  layout->addWidget(formWidget);

  //! Add sliders for spherical angles alpha and beta for light direction.
  alphaSlider.setOrientation(Qt::Horizontal);
  betaSlider.setOrientation(Qt::Horizontal);
  formLayout->addRow("alpha", &alphaSlider);
  formLayout->addRow("beta", &betaSlider);

  //! Connect signals and slots for sliders.
  connect(&alphaSlider, SIGNAL(valueChanged(int)), this, SLOT(alphaBetaSliderValueChanged()));
  connect(&betaSlider, SIGNAL(valueChanged(int)), this, SLOT(alphaBetaSliderValueChanged()));
}

void LightEditor::alphaBetaSliderValueChanged() {

  //! Get alpha value in [-180, 180] degrees.
  float alpha = -180.0f + float(alphaSlider.value() - alphaSlider.minimum()) / float(alphaSlider.maximum() - alphaSlider.minimum()) * 360.0f;

  //! Get beta value in [-90, 90] degrees.
  float beta = -90.0f + float(betaSlider.value() - betaSlider.minimum()) / float(betaSlider.maximum() - betaSlider.minimum()) * 180.0f;

  //! Compute unit vector.
  float lightX = cos(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
  float lightY = sin(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
  float lightZ = sin(beta * M_PI/180.0f);

  //! Update OSPRay light direction.
  ospSet3f(light, "direction", lightX, lightY, lightZ);

  //! Commit and emit signal.
  ospCommit(light);
  emit lightChanged();
}
