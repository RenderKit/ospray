// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "QOSPRayWindow.h"

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
}

QOSPRayWindow::~QOSPRayWindow()
{
  // free the frame buffer and camera
  // we don't own the renderer!
  if(frameBuffer)
    {
      ospFreeFrameBuffer(frameBuffer);
    }

  if(camera)
    {
      ospRelease(camera);
    }
}

void QOSPRayWindow::setRenderingEnabled(bool renderingEnabled)
{
  this->renderingEnabled = renderingEnabled;

  // trigger render if true
  if(renderingEnabled == true)
    {
      updateGL();
    }
}

void QOSPRayWindow::setRotationRate(float rotationRate)
{
  this->rotationRate = rotationRate;
}

void QOSPRayWindow::setBenchmarkParameters(int benchmarkWarmUpFrames, int benchmarkFrames)
{
  this->benchmarkWarmUpFrames = benchmarkWarmUpFrames;
  this->benchmarkFrames = benchmarkFrames;
}

void QOSPRayWindow::setWorldBounds(const osp::box3f &worldBounds)
{
  this->worldBounds = worldBounds;

  // set viewport look at point to center of world bounds
  viewport.at = center(worldBounds);

  // set viewport from point relative to center of world bounds
  viewport.from = viewport.at - 1.5f * viewport.frame.l.vy;

  updateGL();
}

void QOSPRayWindow::paintGL()
{
  if(!renderingEnabled || !frameBuffer || !renderer)
    {
      return;
    }

  // if we're benchmarking and we've completed the required number of warm-up frames, start the timer
  if(benchmarkFrames > 0 && frameCount == benchmarkWarmUpFrames)
    {
      std::cout << "starting benchmark timer" << std::endl;
      benchmarkTimer.start();
    }

  // update OSPRay camera if viewport has been modified
  if(viewport.modified)
    {
      ospSetVec3f(camera,"pos" ,viewport.from);
      ospSetVec3f(camera,"dir" ,viewport.at - viewport.from);
      ospSetVec3f(camera,"up", viewport.up);
      ospSetf(camera,"aspect", viewport.aspect);
      ospSetf(camera,"fovy", viewport.fovY);

      ospCommit(camera);

      viewport.modified = false;
    }

  renderFrameTimer.start();
  ospRenderFrame(frameBuffer, renderer);
  double framesPerSecond = 1000.0 / renderFrameTimer.elapsed();
  char title[1024];  sprintf(title, "OSPRay Volume Viewer (%.4f fps)", framesPerSecond);
  if (showFrameRate == true) parent->setWindowTitle(title);

  uint32 *mappedFrameBuffer = (unsigned int *) ospMapFrameBuffer(frameBuffer);

  glDrawPixels(windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, mappedFrameBuffer);
  if (writeFramesFilename.length()) writeFrameBufferToFile(mappedFrameBuffer);

  ospUnmapFrameBuffer(mappedFrameBuffer, frameBuffer);

  // automatic rotation
  if(rotationRate != 0.f)
    {
      rotateCenter(rotationRate, 0.f);
    }

  // increment frame counter
  frameCount++;

  // quit if we're benchmarking and have exceeded the needed number of frames
  if(benchmarkFrames > 0 && frameCount >= benchmarkWarmUpFrames + benchmarkFrames)
    {
      float elapsedSeconds = float(benchmarkTimer.elapsed()) / 1000.f;

      std::cout << "benchmark: " << elapsedSeconds << " elapsed seconds ==> " << float(benchmarkFrames) / elapsedSeconds << " fps" << std::endl;

      QCoreApplication::quit();
    }

  // force continuous rendering if we have automatic rotation or benchmarking enabled
  if(rotationRate != 0.f || benchmarkFrames > 0)
    {
      update();
    }
}

void QOSPRayWindow::resizeGL(int width, int height)
{
  windowSize = osp::vec2i(width, height);

  // reallocate OSPRay framebuffer for new size
  if(frameBuffer)
    {
      ospFreeFrameBuffer(frameBuffer);
    }

  frameBuffer = ospNewFrameBuffer(windowSize, OSP_RGBA_I8);

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
}

void QOSPRayWindow::mouseMoveEvent(QMouseEvent * event)
{
  int dx = event->x() - lastMousePosition.x();
  int dy = event->y() - lastMousePosition.y();

  if(event->buttons() & Qt::LeftButton)
    {
      // camera rotation about center point
      const float rotationSpeed = 0.003f;

      float du = dx * rotationSpeed;
      float dv = dy * rotationSpeed;

      rotateCenter(du, dv);
    }
  else if(event->buttons() & Qt::RightButton)
    {
      // camera distance from center point
      const float motionSpeed = 0.012f;

      float forward = dy * motionSpeed;
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
}

void QOSPRayWindow::rotateCenter(float du, float dv)
{
  const osp::vec3f pivot = center(worldBounds);

  osp::affine3f xfm = osp::affine3f::translate(pivot)
    * osp::affine3f::rotate(viewport.frame.l.vx, -dv)
    * osp::affine3f::rotate(viewport.frame.l.vz, -du)
    * osp::affine3f::translate(-pivot);

  viewport.frame = xfm * viewport.frame;
  viewport.from  = xfmPoint(xfm, viewport.from);
  viewport.at    = xfmPoint(xfm, viewport.at);
  viewport.snapUp();

  viewport.modified = true;
}

void QOSPRayWindow::writeFrameBufferToFile(const uint32 *pixelData)
{

  static uint32 frameNumber = 0;
  char filename[1024];
  sprintf(filename, "%s_%05u.ppm", writeFramesFilename.c_str(), frameNumber++);
  FILE *file = fopen(filename, "wb");  if (!file) { std::cerr << "unable to write to file '" << filename << "'" << std::endl;  return; }

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

