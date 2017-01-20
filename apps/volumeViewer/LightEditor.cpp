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

#include "LightEditor.h"

LightEditor::LightEditor(OSPLight ambientLight, OSPLight directionalLight) : ambientLight(ambientLight),
                                                                             directionalLight(directionalLight)
{
  // Make sure we have existing lights.
  if (!ambientLight || !directionalLight)
    throw std::runtime_error("LightEditor: must be constructed with an existing lights");

  // Setup UI elments.
  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);

  // Form layout.
  QWidget * formWidget = new QWidget();
  QFormLayout * formLayout = new QFormLayout();
  formWidget->setLayout(formLayout);
  layout->addWidget(formWidget);

  // Ambient light intensity.
  ambientLightIntensitySpinBox.setRange(0.0, 3.0);
  ambientLightIntensitySpinBox.setSingleStep(0.01);
  formLayout->addRow("Ambient light intensity", &ambientLightIntensitySpinBox);
  connect(&ambientLightIntensitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(ambientLightChanged()));

  // Directional light intensity.
  directionalLightIntensitySpinBox.setRange(0.0, 3.0);
  directionalLightIntensitySpinBox.setSingleStep(0.01);
  formLayout->addRow("Directional light intensity", &directionalLightIntensitySpinBox);
  connect(&directionalLightIntensitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(directionalLightChanged()));

  // Directional light: azimuth and elevation angles for direction.
  directionalLightAzimuthSlider.setOrientation(Qt::Horizontal);
  directionalLightElevationSlider.setOrientation(Qt::Horizontal);
  formLayout->addRow("Directional light azimuth", &directionalLightAzimuthSlider);
  formLayout->addRow("Directional light elevation", &directionalLightElevationSlider);
  connect(&directionalLightAzimuthSlider, SIGNAL(valueChanged(int)), this, SLOT(directionalLightChanged()));
  connect(&directionalLightElevationSlider, SIGNAL(valueChanged(int)), this, SLOT(directionalLightChanged()));

  // Set default light parameters.
  ambientLightIntensitySpinBox.setValue(0.1);  //doesn't seem to fire if it's 0 first
  ambientLightIntensitySpinBox.setValue(0.2);

  directionalLightIntensitySpinBox.setValue(1.7);
  directionalLightAzimuthSlider.setValue(0.8 * (directionalLightAzimuthSlider.minimum() + directionalLightAzimuthSlider.maximum())); // 45 degrees azimuth
  directionalLightElevationSlider.setValue(0.65 * (directionalLightElevationSlider.minimum() + directionalLightElevationSlider.maximum())); // 45 degrees elevation
}

void LightEditor::ambientLightChanged()
{
  ospSet1f(ambientLight, "intensity", float(ambientLightIntensitySpinBox.value()*3.14));
  ospCommit(ambientLight);
  emit lightsChanged();
}

void LightEditor::directionalLightChanged()
{
  ospSet1f(directionalLight, "intensity", float(directionalLightIntensitySpinBox.value()*3.14));

  // Get alpha value in [-180, 180] degrees.
  float alpha = -180.0f + float(directionalLightAzimuthSlider.value() - directionalLightAzimuthSlider.minimum()) / float(directionalLightAzimuthSlider.maximum() - directionalLightAzimuthSlider.minimum()) * 360.0f;

  // Get beta value in [-90, 90] degrees.
  float beta = -90.0f + float(directionalLightElevationSlider.value() - directionalLightElevationSlider.minimum()) / float(directionalLightElevationSlider.maximum() - directionalLightElevationSlider.minimum()) * 180.0f;

  // Compute unit vector.
  float lightX = cos(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
  float lightY = sin(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
  float lightZ = sin(beta * M_PI/180.0f);

  // Update OSPRay light direction.
  ospSet3f(directionalLight, "direction", lightX, lightY, lightZ);

  // Commit and emit signal.
  ospCommit(directionalLight);
  emit lightsChanged();
}
