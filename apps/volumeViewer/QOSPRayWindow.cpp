#include "QOSPRayWindow.h"

QOSPRayWindow::QOSPRayWindow(OSPRenderer renderer) : renderingEnabled_(false), frameBuffer_(NULL), renderer_(NULL), camera_(NULL)
{
    // assign renderer
    assert(renderer);
    renderer_ = renderer;

    // setup camera
    camera_ = ospNewCamera("perspective");
    assert(camera_ != NULL);

    float distance = 0.5;
    osp::vec3f pos(-distance, distance, -distance);
    osp::vec3f center(0.5, 0.5, 0.5);
    osp::vec3f dir = center - pos;

    ospSet3f(camera_,"pos", pos.x, pos.y, pos.z);
    ospSet3f(camera_,"dir", dir.x, dir.y, dir.z);
    ospSet3f(camera_,"up", 0., 0., 1.);
    ospSetf(camera_,"aspect",float(width()) / float(height()));

    ospCommit(camera_);

    ospSetParam(renderer_, "camera", camera_);
}

QOSPRayWindow::~QOSPRayWindow()
{
    // free the frame buffer and camera
    // we don't own the renderer!
    if(frameBuffer_)
    {
        ospFreeFrameBuffer(frameBuffer_);
    }

    if(camera_)
    {
        ospRelease(camera_);
    }
}

void QOSPRayWindow::setRenderingEnabled(bool renderingEnabled)
{
    renderingEnabled_ = renderingEnabled;

    // trigger render if true
    if(renderingEnabled_ == true)
    {
        updateGL();
    }
}

OSPFrameBuffer QOSPRayWindow::getFrameBuffer()
{
    return frameBuffer_;
}

void QOSPRayWindow::paintGL()
{
    if(!renderingEnabled_ || !frameBuffer_ || !renderer_)
    {
        return;
    }

    ospRenderFrame(frameBuffer_, renderer_);

    uint32 * mappedFrameBuffer = (unsigned int *)ospMapFrameBuffer(frameBuffer_);

    glDrawPixels(windowSize_.x, windowSize_.y, GL_RGBA, GL_UNSIGNED_BYTE, mappedFrameBuffer);

    ospUnmapFrameBuffer(mappedFrameBuffer, frameBuffer_);
}

void QOSPRayWindow::resizeGL(int width, int height)
{
    windowSize_ = osp::vec2i(width, height);

    // reallocate OSPRay framebuffer for new size
    if(frameBuffer_)
    {
        ospFreeFrameBuffer(frameBuffer_);
    }

    frameBuffer_ = ospNewFrameBuffer(windowSize_, OSP_RGBA_I8);

    // update camera aspect ratio
    ospSetf(camera_, "aspect", float(width) / float(height));
    ospCommit(camera_);

    // update OpenGL viewport and force redraw
    glViewport(0, 0, width, height);
    updateGL();
}
