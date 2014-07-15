/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "QOSPRayWindow.h"
#include <QtGui>

class VolumeViewer : public QMainWindow
{
Q_OBJECT

public:

    VolumeViewer();

    void loadVolume(const std::string &filename, const osp::vec3i &dimensions, const std::string &format, const std::string &layout, const float dt);

public slots:

    void render();

protected:

    void createTransferFunction();

    // OSPRay objects
    OSPRenderer renderer_;
    OSPVolume volume_;
    OSPTransferFunction transferFunction_;

    // the view window
    QOSPRayWindow * osprayWindow_;
};
