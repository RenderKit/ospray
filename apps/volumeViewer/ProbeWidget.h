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
class QOSPRayWindow;

class ProbeCoordinateWidget : public QWidget {

Q_OBJECT

public:

  ProbeCoordinateWidget(const std::string &label, ospcommon::vec2f range);

  float getValue() { return spinBox.value(); }

signals:

  void probeCoordinateChanged();

protected slots:

  void updateValue();

protected:

  ospcommon::vec2f range;

  QSlider slider;
  QDoubleSpinBox spinBox;
};


class ProbeWidget : public QWidget {

Q_OBJECT

public:

  ProbeWidget(VolumeViewer *volumeViewer);

signals:

  void enabled(bool);
  void probeChanged();

public slots:

  void setEnabled(bool value);
  void setVolume(OSPVolume volume) { this->volume = volume; emit(probeChanged()); }
  void updateProbe();
  void render();

protected:

  //! The parent volume viewer.
  VolumeViewer *volumeViewer;

  //! The OSPRay window.
  QOSPRayWindow *osprayWindow;

  //! Bounding box to consider.
  ospcommon::box3f boundingBox;

  //! Active volume to probe.
  OSPVolume volume;

  //! Whether the probe is enabled.
  bool isEnabled;

  //! The probe coordinate.
  osp::vec3f coordinate;

  // UI elements.
  std::vector<ProbeCoordinateWidget *> probeCoordinateWidgets;
  QLabel sampleValueLabel;
};
