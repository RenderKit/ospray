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
  gradientShadingEnabledCheckBox = new QCheckBox();
  connect(gradientShadingEnabledCheckBox, SIGNAL(toggled(bool)), volumeViewer, SLOT(setGradientShadingEnabled(bool)));
  formLayout->addRow("Volume gradient shading (lighting)", gradientShadingEnabledCheckBox);

  adaptiveSamplingCB = new QCheckBox();
  connect(adaptiveSamplingCB, SIGNAL(toggled(bool)), volumeViewer, SLOT(setAdaptiveSampling(bool)));
  formLayout->addRow("Adaptive sampling", adaptiveSamplingCB);

  preIntegrationCB = new QCheckBox();
  connect(preIntegrationCB, SIGNAL(toggled(bool)), volumeViewer, SLOT(setPreIntegration(bool)));
  formLayout->addRow("PreIntegration", preIntegrationCB);

  QCheckBox *singleShadeCB = new QCheckBox();
  connect(singleShadeCB, SIGNAL(toggled(bool)), volumeViewer, SLOT(setSingleShade(bool)));
  formLayout->addRow("Single Shading Calculation", singleShadeCB);

  shadowsCB = new QCheckBox();
  connect(shadowsCB, SIGNAL(toggled(bool)), volumeViewer, SLOT(setShadows(bool)));
  formLayout->addRow("Shadows", shadowsCB);

  planeCB = new QCheckBox();
  connect(planeCB, SIGNAL(toggled(bool)), volumeViewer, SLOT(setPlane(bool)));
  formLayout->addRow("Plane", planeCB);


  QDoubleSpinBox *adaptiveScalarSB = new QDoubleSpinBox();
  adaptiveScalarSB->setDecimals(4);
  adaptiveScalarSB->setRange(1.0, 1000.0);
  adaptiveScalarSB->setSingleStep(1.0);
  connect(adaptiveScalarSB, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setAdaptiveScalar(double)));
  formLayout->addRow("Adaptive scalar", adaptiveScalarSB);

  adaptiveMaxSamplingRateSB = new QDoubleSpinBox();
  adaptiveMaxSamplingRateSB->setDecimals(3);
  adaptiveMaxSamplingRateSB->setRange(0.01, 16.0);
  adaptiveMaxSamplingRateSB->setSingleStep(0.01);
  connect(adaptiveMaxSamplingRateSB, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setAdaptiveMaxSamplingRate(double)));
  formLayout->addRow("Adaptive max sampling rate", adaptiveMaxSamplingRateSB);

  QDoubleSpinBox *adaptiveBacktrackSB = new QDoubleSpinBox();
  adaptiveBacktrackSB->setDecimals(4);
  adaptiveBacktrackSB->setRange(0.0001, 2.0f);
  adaptiveBacktrackSB->setSingleStep(0.0001);
  connect(adaptiveBacktrackSB, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setAdaptiveBacktrack(double)));
  formLayout->addRow("Adaptive backtrack", adaptiveBacktrackSB);

  // sampling rate selection
  samplingRateSpinBox = new QDoubleSpinBox();
  samplingRateSpinBox->setDecimals(3);
  samplingRateSpinBox->setRange(0.01, 16.0);
  samplingRateSpinBox->setSingleStep(0.01);
  connect(samplingRateSpinBox, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setSamplingRate(double)));
  formLayout->addRow("Sampling rate", samplingRateSpinBox);

  QDoubleSpinBox *aoWeightSB = new QDoubleSpinBox();
  aoWeightSB->setDecimals(3);
  aoWeightSB->setRange(0.0, 4.0);
  aoWeightSB->setSingleStep(0.1);
  connect(aoWeightSB, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setAOWeight(double)));
  formLayout->addRow("AOWeight", aoWeightSB);

  aoSamplesSB = new QSpinBox();
  aoSamplesSB->setRange(0, 64);
  connect(aoSamplesSB, SIGNAL(valueChanged(int)), volumeViewer, SLOT(setAOSamples(int)));
  formLayout->addRow("AOSamples", aoSamplesSB);

  sppSB = new QSpinBox();
  sppSB->setRange(-4, 2048);
  connect(sppSB, SIGNAL(valueChanged(int)), volumeViewer, SLOT(setSPP(int)));
  formLayout->addRow("spp", sppSB);

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
  gradientShadingEnabledCheckBox->setChecked(volumeViewer->getGradientShadingEnabled());
  singleShadeCB->setChecked(true);
  adaptiveSamplingCB->setChecked(volumeViewer->getAdaptiveSampling());
  preIntegrationCB->setChecked(volumeViewer->getPreIntegration());
  shadowsCB->setChecked(volumeViewer->getShadows());
  planeCB->setChecked(volumeViewer->getPlane());
  adaptiveScalarSB->setValue(15.f);
  adaptiveMaxSamplingRateSB->setValue(volumeViewer->getAdaptiveMaxSamplingRate());
  adaptiveBacktrackSB->setValue(0.02f);
  samplingRateSpinBox->setValue(volumeViewer->getSamplingRate());
  aoWeightSB->setValue(0.4f);
  aoSamplesSB->setValue(volumeViewer->getAOSamples());
  sppSB->setValue(volumeViewer->getSPP());
}

void PreferencesDialog::setGradientShadingEnabled(bool v)
{
  if (gradientShadingEnabledCheckBox)
    gradientShadingEnabledCheckBox->setChecked(v);
}

void PreferencesDialog::setSPP(int v)
{
  sppSB->setValue(v);
}

void PreferencesDialog::setAOSamples(int v)
{
  aoSamplesSB->setValue(v);
}

void PreferencesDialog::setPreIntegration(bool v)
{
  preIntegrationCB->setChecked(v);
}

void PreferencesDialog::setShadows(bool v)
{
  shadowsCB->setChecked(v);
}

void PreferencesDialog::setAdaptiveSampling(bool v)
{
  adaptiveSamplingCB->setChecked(v);
}

void PreferencesDialog::setPlane(bool v)
{
  planeCB->setChecked(v);
}

void PreferencesDialog::setSamplingRate(float v)
{
  samplingRateSpinBox->setValue(v);
}
void PreferencesDialog::setAdaptiveMaxSamplingRate(float v)
{
  adaptiveMaxSamplingRateSB->setValue(v);
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
