#pragma once

#include <QGLWidget>
#include <ospray/ospray.h>

class QOSPRayWindow : public QGLWidget
{
public:

    QOSPRayWindow(OSPRenderer renderer);
    virtual ~QOSPRayWindow();

    void setRenderingEnabled(bool renderingEnabled);

    OSPFrameBuffer getFrameBuffer();

protected:

    virtual void paintGL();
    virtual void resizeGL(int width, int height);

    // only render when this flag is true
    // this allows the window to be created before all required components are ospCommit()'d
    bool renderingEnabled_;

    osp::vec2i windowSize_;

    OSPFrameBuffer frameBuffer_;
    OSPRenderer renderer_;
    OSPCamera camera_;
};
