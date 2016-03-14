// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "VolumeViewer.h"
#include "PreferencesDialog.h"

PreferencesDialog::PreferencesDialog(VolumeViewer *volumeViewer, ospcommon::box3f boundingBox) : QDialog(volumeViewer), volumeViewer(volumeViewer)
{
  setWindowTitle("Preferences");

  QFormLayout *formLayout = new QFormLayout();
  setLayout(formLayout);

  // render annotations flag
  QCheckBox *renderAnnotationsEnabledCheckBox = new QCheckBox();
  connect(renderAnnotationsEnabledCheckBox, SIGNAL(toggled(bool)), volumeViewer, SLOT(setRenderAnnotationsEnabled(bool)));
  formLayout->addRow("Render annotations", renderAnnotationsEnabledCheckBox);

  // subsampling during interaction flag
  QCheckBox *subsamplingInteractionEnabledCheckBox = new QCheckBox();
  connect(subsamplingInteractionEnabledCheckBox, SIGNAL(toggled(bool)), volumeViewer, SLOT(setSubsamplingInteractionEnabled(bool)));
  formLayout->addRow("Subsample during interaction", subsamplingInteractionEnabledCheckBox);

  // gradient shading flag
  QCheckBox *gradientShadingEnabledCheckBox = new QCheckBox();
  connect(gradientShadingEnabledCheckBox, SIGNAL(toggled(bool)), volumeViewer, SLOT(setGradientShadingEnabled(bool)));
  formLayout->addRow("Volume gradient shading (lighting)", gradientShadingEnabledCheckBox);

  // sampling rate selection
  QDoubleSpinBox *samplingRateSpinBox = new QDoubleSpinBox();
  samplingRateSpinBox->setDecimals(3);
  samplingRateSpinBox->setRange(0.01, 1.0);
  samplingRateSpinBox->setSingleStep(0.01);
  connect(samplingRateSpinBox, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setSamplingRate(double)));
  formLayout->addRow("Sampling rate", samplingRateSpinBox);

  // volume clipping box
  for(size_t i=0; i<6; i++) {
    volumeClippingBoxSpinBoxes.push_back(new QDoubleSpinBox());

    volumeClippingBoxSpinBoxes[i]->setDecimals(3);
    volumeClippingBoxSpinBoxes[i]->setRange(boundingBox.lower[i % 3], boundingBox.upper[i % 3]);
    volumeClippingBoxSpinBoxes[i]->setSingleStep(0.01 * (boundingBox.upper[i % 3] - boundingBox.lower[i % 3]));

    if(i < 3)
      volumeClippingBoxSpinBoxes[i]->setValue(boundingBox.lower[i % 3]);
    else
      volumeClippingBoxSpinBoxes[i]->setValue(boundingBox.upper[i % 3]);

    connect(volumeClippingBoxSpinBoxes[i], SIGNAL(valueChanged(double)), this, SLOT(updateVolumeClippingBox()));
  }

  QWidget *volumeClippingBoxLowerWidget = new QWidget();
  QHBoxLayout *hBoxLayout = new QHBoxLayout();
  volumeClippingBoxLowerWidget->setLayout(hBoxLayout);

  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[0]);
  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[1]);
  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[2]);

  formLayout->addRow("Volume clipping box: lower", volumeClippingBoxLowerWidget);

  QWidget *volumeClippingBoxUpperWidget = new QWidget();
  hBoxLayout = new QHBoxLayout();
  volumeClippingBoxUpperWidget->setLayout(hBoxLayout);

  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[3]);
  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[4]);
  hBoxLayout->addWidget(volumeClippingBoxSpinBoxes[5]);

  formLayout->addRow("Volume clipping box: upper", volumeClippingBoxUpperWidget);

  // set default values. this will trigger signal / slot executions.
  subsamplingInteractionEnabledCheckBox->setChecked(false);
  gradientShadingEnabledCheckBox->setChecked(false);
  samplingRateSpinBox->setValue(0.125);
}

void PreferencesDialog::updateVolumeClippingBox()
{
  ospcommon::vec3f lower(volumeClippingBoxSpinBoxes[0]->value(), 
                      volumeClippingBoxSpinBoxes[1]->value(), 
                      volumeClippingBoxSpinBoxes[2]->value());
  ospcommon::vec3f upper(volumeClippingBoxSpinBoxes[3]->value(), 
                      volumeClippingBoxSpinBoxes[4]->value(), 
                      volumeClippingBoxSpinBoxes[5]->value());

  volumeViewer->setVolumeClippingBox(ospcommon::box3f(lower, upper));
}
