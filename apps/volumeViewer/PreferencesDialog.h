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
#include "ospcommon/box.h"
#include <QtGui>
#include <vector>

class VolumeViewer;

class PreferencesDialog : public QDialog
{
Q_OBJECT

public:

  PreferencesDialog(VolumeViewer *volumeViewer, ospcommon::box3f boundingBox);

  void setSamplingRate(float v);
  void setAdaptiveMaxSamplingRate(float v);
public slots:

void setGradientShadingEnabled(bool v);
void setSPP(int v);
void setAOSamples(int v);
void setPreIntegration(bool v);
void setShadows(bool v);
void setAdaptiveSampling(bool v);
void setPlane(bool v);


protected slots:

  void updateVolumeClippingBox();

protected:

  VolumeViewer *volumeViewer;
  QDoubleSpinBox* samplingRateSpinBox;
  QDoubleSpinBox* adaptiveMaxSamplingRateSB;
  QSpinBox* sppSB;
  QSpinBox* aoSamplesSB;
  QCheckBox* preIntegrationCB;
  QCheckBox* adaptiveSamplingCB;
  QCheckBox* shadowsCB;
  QCheckBox* gradientShadingEnabledCheckBox;
  QCheckBox* planeCB;

  std::vector<QDoubleSpinBox *> volumeClippingBoxSpinBoxes;
};
