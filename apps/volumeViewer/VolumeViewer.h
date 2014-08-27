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
#include <vector>

class VolumeViewer : public QMainWindow
{
Q_OBJECT

public:

    VolumeViewer(const osp::vec3i &dimensions, const float dt);

    QOSPRayWindow *getQOSPRayWindow() { return osprayWindow_; }

    void loadVolume(const std::string &filename, const osp::vec3i &dimensions, const std::string &format, const std::string &layout);

    void setVolume(size_t index);

public slots:

    void render();
    void nextTimestep();
    void playTimesteps(bool set);

protected:

    void createTransferFunction();

    // OSPRay objects
    OSPRenderer renderer_;
    std::vector<OSPVolume> volumes_;
    OSPTransferFunction transferFunction_;

    // the view window
    QOSPRayWindow * osprayWindow_;

    // play timesteps timer
    QTimer playTimestepsTimer_;
};
