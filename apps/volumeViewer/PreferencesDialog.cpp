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

#include "PreferencesDialog.h"
#include "VolumeViewer.h"

PreferencesDialog::PreferencesDialog(VolumeViewer *volumeViewer) : QDialog(volumeViewer)
{
  setWindowTitle("Preferences");

  QFormLayout *formLayout = new QFormLayout();
  setLayout(formLayout);

  // sampling rate selection
  QDoubleSpinBox *samplingRateSpinBox = new QDoubleSpinBox();
  samplingRateSpinBox->setDecimals(3);
  samplingRateSpinBox->setRange(0.01, 1.0);
  samplingRateSpinBox->setSingleStep(0.01);
  connect(samplingRateSpinBox, SIGNAL(valueChanged(double)), volumeViewer, SLOT(setSamplingRate(double)));
  formLayout->addRow("Sampling rate", samplingRateSpinBox);

  // set default value. this will trigger signal / slot executions.
  samplingRateSpinBox->setValue(0.125);
}
