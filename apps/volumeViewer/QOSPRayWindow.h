/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include <QtGui>
#include <QGLWidget>
#include <ospray/ospray.h>

struct Viewport
{
    Viewport() : from(0,-1,0),
                 at(0,0,0),
                 up(0,0,1),
                 aspect(1.f),
                 fovY(60.f),
                 modified(true)
    {
        frame = osp::affine3f::translate(from) * osp::affine3f(embree::one);
    }

    osp::vec3f from;
    osp::vec3f at;
    osp::vec3f up;

    /*! aspect ratio (width / height) */
    float aspect;

    /*! vertical field of view (degrees) */
    float fovY;

    /*! this flag should be set every time the viewport is modified */
    bool modified;

    /*! camera frame in which the Y axis is the depth axis, and X
    and Z axes are parallel to the screen X and Y axis. The frame
    itself remains normalized. */
    osp::affine3f frame;

    /*! set up vector */
    void setUp(const osp::vec3f &vec)
    {
        up = vec;
        snapUp();
        modified = true;
    }

    /*! set frame 'up' vector. if this vector is (0,0,0) the window will
    *not* apply the up-vector after camera manipulation */
    void snapUp()
    {
        if(fabsf(dot(up,frame.l.vz)) < 1e-3f)
            return;

        frame.l.vx = normalize(cross(frame.l.vy,up));
        frame.l.vz = normalize(cross(frame.l.vx,frame.l.vy));
        frame.l.vy = normalize(cross(frame.l.vz,frame.l.vx));
    }
};


class QOSPRayWindow : public QGLWidget
{
public:

    QOSPRayWindow(OSPRenderer renderer);
    virtual ~QOSPRayWindow();

    void setRenderingEnabled(bool renderingEnabled);
    void setRotationRate(float rotationRate);
    void setBenchmarkParameters(int benchmarkWarmUpFrames, int benchmarkFrames);
    virtual void setWorldBounds(const osp::box3f &worldBounds);

    Viewport * getViewport() { return &viewport_; }

    OSPFrameBuffer getFrameBuffer() { return frameBuffer_; }

protected:

    virtual void paintGL();
    virtual void resizeGL(int width, int height);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);

    /*! rotate about center point */
    virtual void rotateCenter(float du, float dv);

    /*! frame counter */
    long frameCount_;

    /*! only render when this flag is true. this allows the window to be created before all required components are ospCommit()'d. */
    bool renderingEnabled_;

    /*! rotation rate to automatically rotate view. */
    float rotationRate_;

    /*! benchmarking: number of warm-up frames */
    int benchmarkWarmUpFrames_;

    /*! benchmarking: number of frames over which to measure frame rate */
    int benchmarkFrames_;

    /*! benchmarking: timer to measure elapsed time over benchmark frames */
    QTime benchmarkTimer_;

    osp::vec2i windowSize_;
    Viewport viewport_;
    osp::box3f worldBounds_;
    QPoint lastMousePosition_;

    OSPFrameBuffer frameBuffer_;
    OSPRenderer renderer_;
    OSPCamera camera_;
};
