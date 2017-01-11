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

#include "SliceWidget.h"
#include "SliceEditor.h"

SliceWidget::SliceWidget(SliceEditor *sliceEditor, ospcommon::box3f boundingBox)
  : boundingBox(boundingBox),
    originSliderAnimationDirection(1) 
{  
  // Check parameters.
  if(ospcommon::volume(boundingBox) <= 0.f)
    throw std::runtime_error("invalid volume bounds");

  // Setup UI elements.
  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);

  // Save and load buttons.
  QWidget * saveLoadWidget = new QWidget();
  QHBoxLayout * hboxLayout = new QHBoxLayout();
  saveLoadWidget->setLayout(hboxLayout);

  QPushButton * saveButton = new QPushButton("Save");
  connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
  hboxLayout->addWidget(saveButton);

  QPushButton * loadButton = new QPushButton("Load");
  connect(loadButton, SIGNAL(clicked()), this, SLOT(load()));
  hboxLayout->addWidget(loadButton);

  layout->addWidget(saveLoadWidget);

  // Form layout for the rest of the UI elements.
  QWidget * formWidget = new QWidget();
  QFormLayout * formLayout = new QFormLayout();
  formWidget->setLayout(formLayout);
  QMargins margins = formLayout->contentsMargins();
  margins.setTop(0); margins.setBottom(0);
  formLayout->setContentsMargins(margins);
  layout->addWidget(formWidget);

  // Origin parameters with default values.
  QWidget * originWidget = new QWidget();
  hboxLayout = new QHBoxLayout();
  originWidget->setLayout(hboxLayout);
  margins = hboxLayout->contentsMargins();
  margins.setTop(0); margins.setBottom(0);
  hboxLayout->setContentsMargins(margins);

  originXSpinBox.setRange(boundingBox.lower.x, boundingBox.upper.x);
  originYSpinBox.setRange(boundingBox.lower.y, boundingBox.upper.y);
  originZSpinBox.setRange(boundingBox.lower.z, boundingBox.upper.z);

  originXSpinBox.setValue(0.5 * (boundingBox.lower.x + boundingBox.upper.x));
  originYSpinBox.setValue(0.5 * (boundingBox.lower.y + boundingBox.upper.y));
  originZSpinBox.setValue(0.5 * (boundingBox.lower.z + boundingBox.upper.z));

  connect(&originXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));
  connect(&originYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));
  connect(&originZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));

  hboxLayout->addWidget(&originXSpinBox);
  hboxLayout->addWidget(&originYSpinBox);
  hboxLayout->addWidget(&originZSpinBox);

  formLayout->addRow("Origin", originWidget);

  // Normal parameters with default values.
  QWidget * normalWidget = new QWidget();
  hboxLayout = new QHBoxLayout();
  normalWidget->setLayout(hboxLayout);
  margins = hboxLayout->contentsMargins();
  margins.setTop(0); margins.setBottom(0);
  hboxLayout->setContentsMargins(margins);

  normalXSpinBox.setRange(-1., 1.);
  normalYSpinBox.setRange(-1., 1.);
  normalZSpinBox.setRange(-1., 1.);

  normalXSpinBox.setValue(1.);
  normalYSpinBox.setValue(0.);
  normalZSpinBox.setValue(0.);

  connect(&normalXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));
  connect(&normalYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));
  connect(&normalZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(apply()));

  hboxLayout->addWidget(&normalXSpinBox);
  hboxLayout->addWidget(&normalYSpinBox);
  hboxLayout->addWidget(&normalZSpinBox);

  formLayout->addRow("Normal", normalWidget);

  // Add a slider and animate button for the origin location along the normal.
  // Defaults to mid-point (matching the origin widgets).
  QWidget * originSliderWidget = new QWidget();
  hboxLayout = new QHBoxLayout();
  originSliderWidget->setLayout(hboxLayout);
  margins = hboxLayout->contentsMargins();
  margins.setTop(0); margins.setBottom(0);
  hboxLayout->setContentsMargins(margins);

  originSlider.setOrientation(Qt::Horizontal);
  originSlider.setValue(0.5 * (originSlider.maximum() - originSlider.minimum()));
  connect(&originSlider, SIGNAL(valueChanged(int)), this, SLOT(originSliderValueChanged(int)));
  hboxLayout->addWidget(&originSlider);

  originSliderAnimateButton.setText("Animate");
  originSliderAnimateButton.setCheckable(true);
  connect(&originSliderAnimateButton, SIGNAL(toggled(bool)), this, SLOT(setAnimation(bool)));
  hboxLayout->addWidget(&originSliderAnimateButton);

  formLayout->addRow("", originSliderWidget);

  // Connect animation timer signal / slot.
  connect(&originSliderAnimationTimer, SIGNAL(timeout()), this, SLOT(animate()));

  // Delete button.
  QWidget * buttonsWidget = new QWidget();
  hboxLayout = new QHBoxLayout();
  buttonsWidget->setLayout(hboxLayout);

  QPushButton * deleteButton = new QPushButton("Delete");
  connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteLater()));
  hboxLayout->addWidget(deleteButton);

  layout->addWidget(buttonsWidget);

  // Frame style for the widget.
  setFrameStyle(QFrame::Panel | QFrame::Raised);

  // Minimum width for the widget.
  setMinimumWidth(240);

  // Connect signals to trigger updates in the slice editor.
  connect(this, SIGNAL(sliceChanged()), sliceEditor, SLOT(apply()));
  connect(this, SIGNAL(sliceDeleted(SliceWidget *)), sliceEditor, SLOT(deleteSlice(SliceWidget *)));

  // Apply slice once event loop continues (allows SliceWidget to be added to SliceEditor list first).
  QTimer::singleShot(0, this, SLOT(apply()));
}

SliceWidget::~SliceWidget()
{
  emit(sliceDeleted(this));
}

SliceParameters SliceWidget::getSliceParameters()
{
  SliceParameters sliceParameters;

  // Get the origin and normal values.
  sliceParameters.origin = ospcommon::vec3f(float(originXSpinBox.value()), float(originYSpinBox.value()), float(originZSpinBox.value()));
  sliceParameters.normal = ospcommon::vec3f(float(normalXSpinBox.value()), float(normalYSpinBox.value()), float(normalZSpinBox.value()));

  return sliceParameters;
}

void SliceWidget::load(std::string filename)
{
  // Get filename if not specified.
  if(filename.empty())
    filename = QFileDialog::getOpenFileName(this, tr("Load slice"), ".", "Slice files (*.slc)").toStdString();

  if(filename.empty())
    return;

  // Get serialized slice state from file.
  QFile file(filename.c_str());
  bool success = file.open(QIODevice::ReadOnly);

  if(!success)
    {
      std::cerr << "unable to open " << filename << std::endl;
      return;
    }

  QDataStream in(&file);

  double originX, originY, originZ;
  in >> originX >> originY >> originZ;

  double normalX, normalY, normalZ;
  in >> normalX >> normalY >> normalZ;

  int sliderPosition;
  in >> sliderPosition;

  bool sliderAnimating;
  in >> sliderAnimating;

  in >> originSliderAnimationDirection;

  // Update slice state. Update values of the UI elements directly to signal appropriate slots.
  originXSpinBox.setValue(originX);
  originYSpinBox.setValue(originY);
  originZSpinBox.setValue(originZ);
  normalXSpinBox.setValue(normalX);
  normalYSpinBox.setValue(normalY);
  normalZSpinBox.setValue(normalZ);
  originSlider.setValue(sliderPosition);
  originSliderAnimateButton.setChecked(sliderAnimating);
}

void SliceWidget::apply()
{
  emit(sliceChanged());
}

void SliceWidget::save() {

  // Get filename.
  QString filename = QFileDialog::getSaveFileName(this, "Save slice", ".", "Slice files (*.slc)");

  if(filename.isNull())
    return;

  // Make sure the filename has the proper extension.
  if(filename.endsWith(".slc") != true)
    filename += ".slc";

  // Serialize slice state to file.
  QFile file(filename);
  bool success = file.open(QIODevice::WriteOnly);

  if(!success)
    {
      std::cerr << "unable to open " << filename.toStdString() << std::endl;
      return;
    }

  QDataStream out(&file);

  out << originXSpinBox.value() << originXSpinBox.value() << originXSpinBox.value();
  out << normalXSpinBox.value() << normalYSpinBox.value() << normalZSpinBox.value();
  out << originSlider.value();
  out << originSliderAnimateButton.isChecked();
  out << originSliderAnimationDirection;
}

void SliceWidget::originSliderValueChanged(int value) {

  // Slider position in [0, 1].
  float sliderPosition = float(value) / float(originSlider.maximum() - originSlider.minimum());

  // Get origin and (normalized) normal vectors.
  ospcommon::vec3f origin(originXSpinBox.value(), originYSpinBox.value(), originZSpinBox.value());
  ospcommon::vec3f normal = normalize(ospcommon::vec3f(normalXSpinBox.value(), normalYSpinBox.value(), normalZSpinBox.value()));

  // Compute allowed range along normal for the volume bounds.
  ospcommon::vec3f upper(normal.x >= 0.f ? boundingBox.upper.x : boundingBox.lower.x,
                   normal.y >= 0.f ? boundingBox.upper.y : boundingBox.lower.y,
                   normal.z >= 0.f ? boundingBox.upper.z : boundingBox.lower.z);

  ospcommon::vec3f lower(normal.x >= 0.f ? boundingBox.lower.x : boundingBox.upper.x,
                   normal.y >= 0.f ? boundingBox.lower.y : boundingBox.upper.y,
                   normal.z >= 0.f ? boundingBox.lower.z : boundingBox.upper.z);

  float tMax = dot(abs(upper - origin), abs(normal));
  float tMin = -dot(abs(lower - origin), abs(normal));

  // Get t value within this range.
  float t = tMin + sliderPosition * (tMax - tMin);

  // Clamp t within epsilon of the minimum and maximum, to prevent artifacts rendering near the bounds of the volume.
  const float epsilon = 0.01;
  t = std::max(std::min(t, tMax-epsilon), tMin+epsilon);

  // Compute updated origin, clamped within the volume bounds.
  ospcommon::vec3f updatedOrigin = clamp(origin + t*normal, boundingBox.lower, boundingBox.upper);

  // Finally, set the new origin value.
  originXSpinBox.setValue(updatedOrigin.x);
  originYSpinBox.setValue(updatedOrigin.y);
  originZSpinBox.setValue(updatedOrigin.z);
}

void SliceWidget::setAnimation(bool set) {

  // Start or stop the animation timer.
  if(set)
    originSliderAnimationTimer.start(100);
  else
    originSliderAnimationTimer.stop();
}

void SliceWidget::animate() {

  int value = originSlider.value();

  // Check if we need to reverse direction.
  if(value + originSliderAnimationDirection < originSlider.minimum() || value + originSliderAnimationDirection > originSlider.maximum())
    originSliderAnimationDirection *= -1;

  // Increment slider position.
  originSlider.setValue(value + originSliderAnimationDirection);
}
