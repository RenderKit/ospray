/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include <QtGui>
#include <ospray/ospray.h>

class SliceWidget : public QWidget
{
Q_OBJECT

public:

    SliceWidget(std::vector<OSPModel> models, osp::box3f volumeBounds);

signals:

    void sliceChanged();

public slots:

    void apply();

protected slots:

    void autoApply();
    void originSliderValueChanged(int value);
    void setAnimation(bool set);
    void animate();

protected:

    //! OSPRay models.
    std::vector<OSPModel> models;

    //! Bounding box of the volume.
    osp::box3f volumeBounds;

    //! OSPRay triangle mesh for the slice.
    OSPTriangleMesh triangleMesh;

    //! UI elements.
    QCheckBox autoApplyCheckBox;

    QDoubleSpinBox originXSpinBox;
    QDoubleSpinBox originYSpinBox;
    QDoubleSpinBox originZSpinBox;

    QDoubleSpinBox normalXSpinBox;
    QDoubleSpinBox normalYSpinBox;
    QDoubleSpinBox normalZSpinBox;

    QSlider originSlider;
    QPushButton originSliderAnimateButton;
    QTimer originSliderAnimationTimer;
};
