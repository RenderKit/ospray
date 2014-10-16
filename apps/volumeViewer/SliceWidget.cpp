/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "SliceWidget.h"

SliceWidget::SliceWidget(std::vector<OSPModel> models, osp::box3f volumeBounds) : models(models),
                                                                                  volumeBounds(volumeBounds),
                                                                                  triangleMesh(NULL),
                                                                                  originSliderAnimationDirection(1) {

    //! Check parameters.
    if(models.size() == 0)
        throw std::runtime_error("must be constructed with existing model(s)");

    if(volume(volumeBounds) <= 0.f)
        throw std::runtime_error("invalid volume bounds");

    //! Setup UI elements.
    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    //! Form layout for the rest of the UI elements.
    QWidget * formWidget = new QWidget();
    QFormLayout * formLayout = new QFormLayout();
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget);

    //! Origin parameters with default values.
    QWidget * originWidget = new QWidget();
    QHBoxLayout * hboxLayout = new QHBoxLayout();
    originWidget->setLayout(hboxLayout);

    originXSpinBox.setRange(volumeBounds.lower.x, volumeBounds.upper.x);
    originYSpinBox.setRange(volumeBounds.lower.y, volumeBounds.upper.y);
    originZSpinBox.setRange(volumeBounds.lower.z, volumeBounds.upper.z);

    originXSpinBox.setValue(0.5 * (volumeBounds.lower.x + volumeBounds.upper.x));
    originYSpinBox.setValue(0.5 * (volumeBounds.lower.y + volumeBounds.upper.y));
    originZSpinBox.setValue(0.5 * (volumeBounds.lower.z + volumeBounds.upper.z));

    connect(&originXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));
    connect(&originYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));
    connect(&originZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));

    hboxLayout->addWidget(&originXSpinBox);
    hboxLayout->addWidget(&originYSpinBox);
    hboxLayout->addWidget(&originZSpinBox);

    formLayout->addRow("Origin", originWidget);

    //! Normal parameters with default values.
    QWidget * normalWidget = new QWidget();
    hboxLayout = new QHBoxLayout();
    normalWidget->setLayout(hboxLayout);

    normalXSpinBox.setRange(-1., 1.);
    normalYSpinBox.setRange(-1., 1.);
    normalZSpinBox.setRange(-1., 1.);

    normalXSpinBox.setValue(1.);
    normalYSpinBox.setValue(0.);
    normalZSpinBox.setValue(0.);

    connect(&normalXSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));
    connect(&normalYSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));
    connect(&normalZSpinBox, SIGNAL(valueChanged(double)), this, SLOT(autoApply()));

    hboxLayout->addWidget(&normalXSpinBox);
    hboxLayout->addWidget(&normalYSpinBox);
    hboxLayout->addWidget(&normalZSpinBox);

    formLayout->addRow("Normal", normalWidget);

    //! Add a slider and animate button for the origin location along the normal.
    QWidget * originSliderWidget = new QWidget();
    hboxLayout = new QHBoxLayout();
    originSliderWidget->setLayout(hboxLayout);

    originSlider.setOrientation(Qt::Horizontal);
    connect(&originSlider, SIGNAL(valueChanged(int)), this, SLOT(originSliderValueChanged(int)));
    hboxLayout->addWidget(&originSlider);

    originSliderAnimateButton.setText("Animate");
    originSliderAnimateButton.setCheckable(true);
    connect(&originSliderAnimateButton, SIGNAL(toggled(bool)), this, SLOT(setAnimation(bool)));
    hboxLayout->addWidget(&originSliderAnimateButton);

    formLayout->addRow("", originSliderWidget);

    //! Connect animation timer signal / slot.
    connect(&originSliderAnimationTimer, SIGNAL(timeout()), this, SLOT(animate()));

    //! Auto-apply checkbox, Delete, and Apply buttons.
    QWidget * buttonsWidget = new QWidget();
    hboxLayout = new QHBoxLayout();
    buttonsWidget->setLayout(hboxLayout);

    autoApplyCheckBox.setText("Auto update");
    autoApplyCheckBox.setChecked(true);
    hboxLayout->addWidget(&autoApplyCheckBox);

    QPushButton * deleteButton = new QPushButton("Delete");
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteLater()));
    hboxLayout->addWidget(deleteButton);

    QPushButton * applyButton = new QPushButton("Apply");
    connect(applyButton, SIGNAL(clicked()), this, SLOT(apply()));
    hboxLayout->addWidget(applyButton);

    layout->addWidget(buttonsWidget);

    //! Frame style for the widget.
    setFrameStyle(QFrame::Panel | QFrame::Raised);

    //! Minimum width for the widget.
    setMinimumWidth(240);

    //! Auto-apply (if enabled)
    autoApply();
}

SliceWidget::~SliceWidget() {

    if(triangleMesh) {

        for(unsigned int i=0; i<models.size(); i++) {

            ospRemoveGeometry(models[i], triangleMesh);
            ospCommit(models[i]);
        }
    }

    emit(sliceChanged());
}

void SliceWidget::apply() {

    //! Get the origin and normal values.
    osp::vec3f origin(float(originXSpinBox.value()), float(originYSpinBox.value()), float(originZSpinBox.value()));
    osp::vec3f normal = osp::vec3f(float(normalXSpinBox.value()), float(normalYSpinBox.value()), float(normalZSpinBox.value()));

    if(normal == osp::vec3f(0.f))
        return;
    else
        normal = normalize(normal);

    //! Create the OSPRay geometry on the first apply().
    bool newGeometry = false;

    if(!triangleMesh) {

        triangleMesh = ospNewTriangleMesh();
        newGeometry = true;
    }

    //! Compute basis vectors.
    osp::vec3f c(1.f,0.f,0.f);

    if(dot(c, normal) > 0.9f)
        c = osp::vec3f(0.f,1.f,0.f);

    osp::vec3f b1 = cross(c, normal);
    osp::vec3f b2 = cross(b1, normal);

    //! For now just make a slice sufficiently large to span the volume.
    const float size = sqrtf(2.f);

    //! Create the triangles forming the slice.
    osp::vec3f lowerLeft(origin - size*b1 - size*b2);
    osp::vec3f span1(2.f*size*b1);
    osp::vec3f span2(2.f*size*b2);

    //! Subdivide the plane into a larger set of triangles to help with performance when combined with other geometries.
    const size_t numSubdivisions = 32;

    std::vector<osp::vec3fa> positions;

    for(size_t i=0; i<numSubdivisions; i++)
        for(size_t j=0; j<numSubdivisions; j++)
            positions.push_back(lowerLeft + float(i)/float(numSubdivisions-1)*span1 + float(j)/float(numSubdivisions-1)*span2);

    std::vector<osp::vec3i> indices;

    for(size_t i=0; i<numSubdivisions-1; i++) {
        for(size_t j=0; j<numSubdivisions-1; j++) {

            indices.push_back(osp::vec3i(i*numSubdivisions+j, (i+1)*numSubdivisions+j, i*numSubdivisions+j+1));
            indices.push_back(osp::vec3i((i+1)*numSubdivisions+j, (i+1)*numSubdivisions+j+1, i*numSubdivisions+j+1));
        }
    }

    //! Update the OSPRay geometry.
    OSPData positionData = ospNewData(positions.size(), OSP_FLOAT3A, &positions[0].x);
    ospSetData(triangleMesh, "position", positionData);

    OSPData indexData = ospNewData(indices.size(), OSP_INT3, &indices[0].x);
    ospSetData(triangleMesh, "index", indexData);

    ospCommit(triangleMesh);

    if(newGeometry)
        for(unsigned int i=0; i<models.size(); i++)
            ospAddGeometry(models[i], triangleMesh);

    for(unsigned int i=0; i<models.size(); i++)
        ospCommit(models[i]);

    emit(sliceChanged());
}

void SliceWidget::autoApply() {

    if(autoApplyCheckBox.isChecked())
        apply();
}

void SliceWidget::originSliderValueChanged(int value) {

    //! Slider position in [0, 1].
    float sliderPosition = float(value) / float(originSlider.maximum() - originSlider.minimum());

    //! Get origin and (normalized) normal vectors.
    osp::vec3f origin(originXSpinBox.value(), originYSpinBox.value(), originZSpinBox.value());
    osp::vec3f normal = normalize(osp::vec3f(normalXSpinBox.value(), normalYSpinBox.value(), normalZSpinBox.value()));

    //! Compute allowed range along normal for the volume bounds.
    osp::vec3f upper(normal.x >= 0.f ? volumeBounds.upper.x : volumeBounds.lower.x,
                     normal.y >= 0.f ? volumeBounds.upper.y : volumeBounds.lower.y,
                     normal.z >= 0.f ? volumeBounds.upper.z : volumeBounds.lower.z);

    osp::vec3f lower(normal.x >= 0.f ? volumeBounds.lower.x : volumeBounds.upper.x,
                     normal.y >= 0.f ? volumeBounds.lower.y : volumeBounds.upper.y,
                     normal.z >= 0.f ? volumeBounds.lower.z : volumeBounds.upper.z);

    float tMax = dot(abs(upper - origin), abs(normal));
    float tMin = -dot(abs(lower - origin), abs(normal));

    //! Get t value within this range.
    float t = tMin + sliderPosition * (tMax - tMin);

    //! Clamp t within epsilon of the minimum and maximum, to prevent artifacts rendering near the bounds of the volume.
    const float epsilon = 0.01;
    t = std::max(std::min(t, tMax-epsilon), tMin+epsilon);

    //! Compute updated origin, clamped within the volume bounds.
    osp::vec3f updatedOrigin = clamp(origin + t*normal, volumeBounds.lower, volumeBounds.upper);

    //! Finally, set the new origin value.
    originXSpinBox.setValue(updatedOrigin.x);
    originYSpinBox.setValue(updatedOrigin.y);
    originZSpinBox.setValue(updatedOrigin.z);
}

void SliceWidget::setAnimation(bool set) {

    //! Start or stop the animation timer.
    if(set)
        originSliderAnimationTimer.start(100);
    else
        originSliderAnimationTimer.stop();
}

void SliceWidget::animate() {

    int value = originSlider.value();

    //! Check if we need to reverse direction.
    if(value + originSliderAnimationDirection < originSlider.minimum() || value + originSliderAnimationDirection > originSlider.maximum())
        originSliderAnimationDirection *= -1;

    //! Increment slider position.
    originSlider.setValue(value + originSliderAnimationDirection);
}
