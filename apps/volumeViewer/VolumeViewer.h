#pragma once

#include "QOSPRayWindow.h"
#include <QtGui>

class VolumeViewer : public QMainWindow
{
Q_OBJECT

public:

    VolumeViewer();

    QOSPRayWindow * getQOSPRayWindow() { return osprayWindow_; }

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
