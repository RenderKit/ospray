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

class LightEditor : public QWidget {

Q_OBJECT

public:

    LightEditor(OSPLight light);

signals:

    void lightChanged();

protected slots:

    void alphaBetaSliderValueChanged();

protected:

    //! OSPRay light.
    OSPLight light;

    //! UI elements.
    QSlider alphaSlider;
    QSlider betaSlider;
};
