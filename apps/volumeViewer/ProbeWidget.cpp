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
#include "ProbeWidget.h"
#include "QOSPRayWindow.h"

#ifdef __APPLE__
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

ProbeCoordinateWidget::ProbeCoordinateWidget(const std::string &label, ospcommon::vec2f range) : range(range)
{
  QHBoxLayout *layout = new QHBoxLayout();
  setLayout(layout);

  spinBox.setRange(range.x, range.y);

  slider.setOrientation(Qt::Horizontal);

  layout->addWidget(new QLabel(label.c_str()));
  layout->addWidget(&slider);
  layout->addWidget(&spinBox);

  connect(&slider, SIGNAL(valueChanged(int)), this, SLOT(updateValue()));
  connect(&spinBox, SIGNAL(valueChanged(double)), this, SLOT(updateValue()));
}

void ProbeCoordinateWidget::updateValue()
{
  if (sender() == &slider) {
    float value = range.x + float(slider.value() - slider.minimum()) / float(slider.maximum() - slider.minimum()) * (range.y - range.x);

    spinBox.blockSignals(true);
    spinBox.setValue(value);
    spinBox.blockSignals(false);
  }
  else if (sender() == &spinBox) {
    float value = spinBox.value();

    slider.blockSignals(true);
    slider.setValue(slider.minimum() + (value - range.x) / (range.y - range.x) * (slider.maximum() - slider.minimum()));
    slider.blockSignals(false);
  }
  else
    throw std::runtime_error("slot executed from unknown sender");

  emit probeCoordinateChanged();
}


ProbeWidget::ProbeWidget(VolumeViewer *volumeViewer) : volumeViewer(volumeViewer), isEnabled(false)
{
  if (!volumeViewer)
    throw std::runtime_error("ProbeWidget: NULL volumeViewer");

  osprayWindow = volumeViewer->getWindow();
  boundingBox = volumeViewer->getBoundingBox();

  if (!osprayWindow)
    throw std::runtime_error("ProbeWidget: NULL osprayWindow");

  // Setup UI elments.
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  setLayout(layout);

  QCheckBox *enabledCheckBox = new QCheckBox("Enable probe");
  layout->addWidget(enabledCheckBox);
  connect(enabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(setEnabled(bool)));

  QGroupBox *coordinatesGroupBox = new QGroupBox("Coordinates");
  QVBoxLayout *coordinatesLayout = new QVBoxLayout();
  coordinatesGroupBox->setLayout(coordinatesLayout);
  connect(this, SIGNAL(enabled(bool)), coordinatesGroupBox, SLOT(setEnabled(bool)));

  probeCoordinateWidgets.push_back(new ProbeCoordinateWidget("x", ospcommon::vec2f(boundingBox.lower.x, boundingBox.upper.x)));
  probeCoordinateWidgets.push_back(new ProbeCoordinateWidget("y", ospcommon::vec2f(boundingBox.lower.y, boundingBox.upper.y)));
  probeCoordinateWidgets.push_back(new ProbeCoordinateWidget("z", ospcommon::vec2f(boundingBox.lower.z, boundingBox.upper.z)));

  for (int i=0; i<3; i++) {
    coordinatesLayout->addWidget(probeCoordinateWidgets[i]);
    connect(probeCoordinateWidgets[i], SIGNAL(probeCoordinateChanged()), this, SIGNAL(probeChanged()));
  }

  layout->addWidget(coordinatesGroupBox);

  layout->addWidget(&sampleValueLabel);
  connect(this, SIGNAL(enabled(bool)), &sampleValueLabel, SLOT(setEnabled(bool)));

  connect(this, SIGNAL(probeChanged()), this, SLOT(updateProbe()), Qt::UniqueConnection);

  // Probe disabled by default
  emit enabled(false);
}

void ProbeWidget::setEnabled(bool value)
{
  isEnabled = value;

  // Connect or disconnect render() slot as needed
  if (value)
    connect(osprayWindow, SIGNAL(renderGLComponents()), this, SLOT(render()), Qt::UniqueConnection);
  else
    disconnect(osprayWindow, SIGNAL(renderGLComponents()), this, SLOT(render()));

  emit probeChanged();
  emit enabled(value);
}

void ProbeWidget::updateProbe()
{
  if (!isEnabled) {
    // force re-render even if disabled: if probe has just been disabled we
    // need to re-render to get rid of the rendered probe
    volumeViewer->render();
    return;
  }

  coordinate = osp::vec3f{probeCoordinateWidgets[0]->getValue(), probeCoordinateWidgets[1]->getValue(), probeCoordinateWidgets[2]->getValue()};

  if (volume) {

    float *results = NULL;
    ospSampleVolume(&results, volume, coordinate, 1);

    if (!results)
      std::cout << "error calling ospSampleVolume()" << std::endl;
    else
      sampleValueLabel.setText("<b>Sampled volume value:</b> " + QString::number(results[0]));
  }
  else
    sampleValueLabel.setText("");

  // Force render to update rendered probe
  volumeViewer->render();
}

void ProbeWidget::render()
{
  const float size = 0.25f * reduce_max(boundingBox.upper - boundingBox.lower);

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);

  glPushMatrix();
  glTranslatef(coordinate.x, coordinate.y, coordinate.z);
  glScalef(size, size, size);

  glBegin(GL_LINES);

  glColor3f(1.f, 0.f, 0.f);
  glVertex3f(-0.5f, 0.f, 0.f);
  glVertex3f( 0.5f, 0.f, 0.f);

  glColor3f(0.f, 1.f, 0.f);
  glVertex3f(0.f, -0.5f, 0.f);
  glVertex3f(0.f,  0.5f, 0.f);

  glColor3f(0.f, 0.f, 1.f);
  glVertex3f(0.f, 0.f, -0.5f);
  glVertex3f(0.f, 0.f,  0.5f);

  glEnd();

  glPopMatrix();
  glPopAttrib();
}
