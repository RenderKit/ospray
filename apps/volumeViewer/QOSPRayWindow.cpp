// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <string>

#include "QOSPRayWindow.h"
#include "modules/opengl/util.h"

#ifdef __APPLE__
  #include <OpenGL/glu.h>
#else
  #include <GL/glu.h>
#endif

std::ostream &operator<<(std::ostream &o, const Viewport &viewport)
{
  o <<  "-vp " << viewport.from.x << " " << viewport.from.y << " " << viewport.from.z
    << " -vi " << viewport.at.x   << " " << viewport.at.y   << " " << viewport.at.z
    << " -vu " << viewport.up.x   << " " << viewport.up.y   << " " << viewport.up.z
    << std::endl;

  return o;
}


QOSPRayWindow::QOSPRayWindow(QMainWindow *parent, 
                             OSPRenderer renderer, 
                             bool showFrameRate,
                             std::string writeFramesFilename)
  : parent(parent), 
    showFrameRate(showFrameRate), 
    frameCount(0), 
    renderingEnabled(false), 
    rotationRate(0.f), 
    benchmarkWarmUpFrames(0), 
    benchmarkFrames(0), 
    frameBuffer(NULL), 
    renderer(NULL), 
    camera(NULL),
    maxDepthTexture(NULL),
    writeFramesFilename(writeFramesFilename)
{
  // assign renderer
  if(!renderer)
    throw std::runtime_error("QOSPRayWindow: must be constructed with an existing renderer");

  this->renderer = renderer;

  // setup camera
  camera = ospNewCamera("perspective");

  if(!camera)
    throw std::runtime_error("QOSPRayWindow: could not create camera type 'perspective'");

  ospCommit(camera);

  ospSetObject(renderer, "camera", camera);

  // connect signals and slots
  connect(&renderTimer, SIGNAL(timeout()), this, SLOT(updateGL()));
  connect(&renderRestartTimer, SIGNAL(timeout()), &renderTimer, SLOT(start()));
}

QOSPRayWindow::~QOSPRayWindow()
{
  // free the frame buffer and camera
  // we don't own the renderer!
  if(frameBuffer)
    ospFreeFrameBuffer(frameBuffer);

  if(camera)
    ospRelease(camera);
}

void QOSPRayWindow::setRenderingEnabled(bool renderingEnabled)
{
  this->renderingEnabled = renderingEnabled;

  // trigger render if true
  if(renderingEnabled == true)
    renderTimer.start();
  else
    renderTimer.stop();
}

void QOSPRayWindow::setRotationRate(float rotationRate)
{
  this->rotationRate = rotationRate;
}

void QOSPRayWindow::setBenchmarkParameters(int benchmarkWarmUpFrames, int benchmarkFrames,
  const std::string &benchmarkFilename)
{
  this->benchmarkWarmUpFrames = benchmarkWarmUpFrames;
  this->benchmarkFrames = benchmarkFrames;
  this->benchmarkFilename = benchmarkFilename;
}

void QOSPRayWindow::setWorldBounds(const ospcommon::box3f &worldBounds)
{
  this->worldBounds = worldBounds;

  // set viewport look at point to center of world bounds
  viewport.at = center(worldBounds);

  // set viewport from point relative to center of world bounds
  viewport.from = viewport.at - 1.5f * length(worldBounds.size()) * viewport.frame.l.vy;

  updateGL();
}

void QOSPRayWindow::paintGL()
{
  if(!renderingEnabled || !frameBuffer || !renderer)
    return;

  // if we're benchmarking and we've completed the required number of warm-up frames, start the timer
  if(benchmarkFrames > 0 && frameCount == benchmarkWarmUpFrames) {
    std::cout << "starting benchmark timer" << std::endl;
    benchmarkTimer.start();
  }

  // update OSPRay camera if viewport has been modified
  if(viewport.modified) {
    const ospcommon::vec3f dir =  viewport.at - viewport.from;
    ospSetVec3f(camera,"pos" ,(const osp::vec3f&)viewport.from);
    ospSetVec3f(camera,"dir" ,(const osp::vec3f&)dir);
    ospSetVec3f(camera,"up", (const osp::vec3f&)viewport.up);
    ospSetf(camera,"aspect", viewport.aspect);
    ospSetf(camera,"fovy", viewport.fovY);

    ospCommit(camera);

    viewport.modified = false;
  }

  renderFrameTimer.start();

  // we have OpenGL components if any slots are connected to the renderGLComponents() signal
  // if so, render these first and then composite the OSPRay-rendered content on top
  bool haveOpenGLComponents = receivers(SIGNAL(renderGLComponents())) > 0;

  if (haveOpenGLComponents) {

    // setup OpenGL view to match current view and render all OpenGL components
    renderGL();

    // generate max depth texture for early ray termination
    if (maxDepthTexture)
      ospRelease(maxDepthTexture);

    maxDepthTexture = ospray::opengl::getOSPDepthTextureFromOpenGLPerspective();
    ospSetObject(renderer, "maxDepthTexture", maxDepthTexture);

    // disable OSPRay background rendering since we're compositing
    ospSet1i(renderer, "backgroundEnabled", 0);

    ospCommit(renderer);

    // disable OpenGL depth testing and enable blending for compositing
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else {

    // unset any maximum depth texture and enable OSPRay background rendering
    ospSetObject(renderer, "maxDepthTexture", NULL);
    ospSet1i(renderer, "backgroundEnabled", 1);
    ospCommit(renderer);

    // disable OpenGL blending
    glDisable(GL_BLEND);
  }

  ospRenderFrame(frameBuffer, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);
  double framesPerSecond = 1000.0 / renderFrameTimer.elapsed();
  char title[1024];  sprintf(title, "OSPRay Volume Viewer (%.4f fps)", framesPerSecond);
  if (showFrameRate == true) parent->setWindowTitle(title);

  uint32_t *mappedFrameBuffer = (unsigned int *) ospMapFrameBuffer(frameBuffer);

  glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, mappedFrameBuffer);
  if (writeFramesFilename.length()) {
    std::string filename = writeFramesFilename + std::to_string(static_cast<long long>(frameCount));
    writeFrameBufferToFile(filename, mappedFrameBuffer);
  }

  ospUnmapFrameBuffer(mappedFrameBuffer, frameBuffer);

  // automatic rotation
  if(rotationRate != 0.f) {
    resetAccumulationBuffer();
    rotateCenter(rotationRate, 0.f);
  }

  // increment frame counter
  frameCount++;

  // quit if we're benchmarking and have exceeded the needed number of frames
  if(benchmarkFrames > 0 && frameCount >= benchmarkWarmUpFrames + benchmarkFrames) {

    float elapsedSeconds = float(benchmarkTimer.elapsed()) / 1000.f;

    std::cout << "benchmark: " << elapsedSeconds << " elapsed seconds ==> "
      << float(benchmarkFrames) / elapsedSeconds << " fps" << std::endl;

    uint32_t *mappedFrameBuffer = (unsigned int *) ospMapFrameBuffer(frameBuffer);

    writeFrameBufferToFile(benchmarkFilename, mappedFrameBuffer);

    QCoreApplication::quit();
  }
}

void QOSPRayWindow::resizeGL(int width, int height)
{
  windowSize = ospcommon::vec2i(width, height);

  // reallocate OSPRay framebuffer for new size
  if(frameBuffer)
    ospFreeFrameBuffer(frameBuffer);

  frameBuffer = ospNewFrameBuffer((const osp::vec2i&)windowSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);

  resetAccumulationBuffer();

  // update viewport aspect ratio
  viewport.aspect = float(width) / float(height);
  viewport.modified = true;

  // update OpenGL viewport and force redraw
  glViewport(0, 0, width, height);
  updateGL();
}

void QOSPRayWindow::mousePressEvent(QMouseEvent * event)
{
  lastMousePosition = event->pos();
}

void QOSPRayWindow::mouseReleaseEvent(QMouseEvent * event)
{
  lastMousePosition = event->pos();

  // restart continuous rendering immediately
  renderTimer.start();
}

void QOSPRayWindow::mouseMoveEvent(QMouseEvent * event)
{
  // pause continuous rendering during interaction and cancel any render restart timers.
  // this keeps interaction more responsive (especially with low frame rates).
  renderTimer.stop();
  renderRestartTimer.stop();

  resetAccumulationBuffer();

  int dx = event->x() - lastMousePosition.x();
  int dy = event->y() - lastMousePosition.y();

  if(event->buttons() & Qt::LeftButton) {

    // camera rotation about center point
    const float rotationSpeed = 0.003f;

    float du = dx * rotationSpeed;
    float dv = dy * rotationSpeed;

    rotateCenter(du, dv);
  }
  else if(event->buttons() & Qt::MidButton) {

    // camera strafe of from / at point
    const float strafeSpeed = 0.001f * length(worldBounds.size());

    float du = dx * strafeSpeed;
    float dv = dy * strafeSpeed;

    strafe(du, dv);
  }
  else if(event->buttons() & Qt::RightButton) {

    // camera distance from center point
    const float motionSpeed = 0.012f;

    float forward = dy * motionSpeed * length(worldBounds.size());
    float oldDistance = length(viewport.at - viewport.from);
    float newDistance = oldDistance - forward;

    if(newDistance < 1e-3f)
      return;

    viewport.from = viewport.at - newDistance * viewport.frame.l.vy;
    viewport.frame.p = viewport.from;

    viewport.modified = true;
  }

  lastMousePosition = event->pos();

  updateGL();

  // after a 0.5s delay, restart continuous rendering.
  renderRestartTimer.setSingleShot(true);
  renderRestartTimer.start(500);
}

void QOSPRayWindow::rotateCenter(float du, float dv)
{
  const ospcommon::vec3f pivot = viewport.at;

  ospcommon::affine3f xfm = ospcommon::affine3f::translate(pivot)
    * ospcommon::affine3f::rotate(viewport.frame.l.vx, -dv)
    * ospcommon::affine3f::rotate(viewport.frame.l.vz, -du)
    * ospcommon::affine3f::translate(-pivot);

  viewport.frame = xfm * viewport.frame;
  viewport.from  = xfmPoint(xfm, viewport.from);
  viewport.at    = xfmPoint(xfm, viewport.at);
  viewport.snapUp();

  viewport.modified = true;
}

void QOSPRayWindow::strafe(float du, float dv)
{
  ospcommon::affine3f xfm = ospcommon::affine3f::translate(dv * viewport.frame.l.vz)
    * ospcommon::affine3f::translate(-du * viewport.frame.l.vx);

  viewport.frame = xfm * viewport.frame;
  viewport.from = xfmPoint(xfm, viewport.from);
  viewport.at = xfmPoint(xfm, viewport.at);
  viewport.modified = true;

  viewport.modified = true;
}

void QOSPRayWindow::renderGL()
{
  // setup OpenGL state to match OSPRay view
  const ospcommon::vec3f bgColor = ospcommon::vec3f(1.f);
  glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.f);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  float zNear = 0.1f;
  float zFar = 100000.f;
  gluPerspective(viewport.fovY, viewport.aspect, zNear, zFar);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(viewport.from.x, viewport.from.y, viewport.from.z,
            viewport.at.x, viewport.at.y, viewport.at.z,
            viewport.up.x, viewport.up.y, viewport.up.z);

  // emit signal to render all OpenGL components; the slots will execute in the order they were registered
  emit(renderGLComponents());
}

void QOSPRayWindow::writeFrameBufferToFile(std::string filename, const uint32_t *pixelData)
{
  filename += ".ppm";
  FILE *file = fopen(filename.c_str(), "wb");
  if (!file) {
    std::cerr << "unable to write to file '" << filename << "'" << std::endl;
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", windowSize.x, windowSize.y);
  unsigned char out[3 * windowSize.x];
  for (int y=0 ; y < windowSize.y ; y++) {
    const unsigned char *in = (const unsigned char *) &pixelData[(windowSize.y - 1 - y) * windowSize.x];
    for (int x=0 ; x < windowSize.x ; x++) {
      out[3 * x + 0] = in[4 * x + 0];
      out[3 * x + 1] = in[4 * x + 1];
      out[3 * x + 2] = in[4 * x + 2];
    }
    fwrite(&out, 3 * windowSize.x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}
