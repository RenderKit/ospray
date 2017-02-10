// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2017 Intel Corporation                                         //
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

#include "OSPGlutViewer.h"

using std::cout;
using std::endl;

using std::string;

using std::lock_guard;
using std::mutex;

using namespace ospcommon;

// Static local helper functions //////////////////////////////////////////////

// helper function to write the rendered image as PPM file
static void writePPM(const string &fileName, const int sizeX, const int sizeY,
                     const uint32_t *pixel)
{
  FILE *file = fopen(fileName.c_str(), "wb");
  fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
  unsigned char *out = (unsigned char *)alloca(3*sizeX);
  for (int y = 0; y < sizeY; y++) {
    const unsigned char *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
    for (int x = 0; x < sizeX; x++) {
      out[3*x + 0] = in[4*x + 0];
      out[3*x + 1] = in[4*x + 1];
      out[3*x + 2] = in[4*x + 2];
    }
    fwrite(out, 3*sizeX, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}

// MSGViewer definitions //////////////////////////////////////////////////////

namespace ospray {

OSPGlutViewer::OSPGlutViewer(const std::deque<box3f> &worldBounds, std::deque<cpp::Model> model,
                             cpp::Renderer renderer, cpp::Camera camera)
  : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
    sceneModels(model),
    frameBuffer(nullptr),
    renderer(renderer),
    camera(camera),
    queuedRenderer(nullptr),
    alwaysRedraw(true),
    fullScreen(false),
    worldBounds(worldBounds),
    lockFirstAnimationFrame(false)
{
  if (!worldBounds.empty()) {
    setWorldBounds(worldBounds[0]);
  }
  renderer.set("world",  sceneModels[0]);
  renderer.set("model",  sceneModels[0]);
  renderer.set("camera", camera);
  renderer.commit();

#if 0
  cout << "#ospGlutViewer: set world bounds " << worldBounds
       << ", motion speed " << motionSpeed << endl;
#endif

  resetAccum = false;
  frameTimer = ospcommon::getSysTime();
  animationTimer = 0.;
  animationFrameDelta = .03;
  animationFrameId = 0;
  animationPaused = false;
  glutViewPort = viewPort;
  scale = vec3f(1,1,1);
}

void OSPGlutViewer::create(const char*  title,
                           const vec2i& size,
                                 bool   fullScreen)
{
  windowTitle = std::string(title);
  Glut3DWidget::create(title, size, fullScreen);
}

void OSPGlutViewer::setRenderer(OSPRenderer renderer)
{
  lock_guard<mutex> lock{rendererMutex};
  queuedRenderer = renderer;
}

void OSPGlutViewer::resetAccumulation()
{
  resetAccum = true;
}

void OSPGlutViewer::toggleFullscreen()
{
  fullScreen = !fullScreen;

  if(fullScreen) {
    glutFullScreen();
  } else {
    glutPositionWindow(0,10);
  }
}

void OSPGlutViewer::resetView()
{
  auto oldAspect = viewPort.aspect;
  viewPort = glutViewPort;
  viewPort.aspect = oldAspect;
}

void OSPGlutViewer::printViewport()
{
  printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
         viewPort.from.x, viewPort.from.y, viewPort.from.z,
         viewPort.up.x,   viewPort.up.y,   viewPort.up.z,
         viewPort.at.x,   viewPort.at.y,   viewPort.at.z);
  fflush(stdout);
}

void OSPGlutViewer::saveScreenshot(const std::string &basename)
{
  const uint32_t *p = (uint32_t*)frameBuffer.map(OSP_FB_COLOR);
  writePPM(basename + ".ppm", windowSize.x, windowSize.y, p);
  cout << "#ospGlutViewer: saved current frame to '" << basename << ".ppm'"
       << endl;
}

void OSPGlutViewer::setWorldBounds(const box3f &worldBounds) {
  Glut3DWidget::setWorldBounds(worldBounds);
  renderer.set("aoDistance", (worldBounds.upper.x - worldBounds.lower.x)/4.f);
  renderer.commit();
}
void OSPGlutViewer::reshape(const vec2i &newSize)
{
  Glut3DWidget::reshape(newSize);
  windowSize = newSize;
  frameBuffer = cpp::FrameBuffer(osp::vec2i{newSize.x, newSize.y},
                                 OSP_FB_SRGBA,
                                 OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);

  frameBuffer.clear(OSP_FB_ACCUM);

  camera.set("aspect", viewPort.aspect);
  camera.commit();
  viewPort.modified = true;
  forceRedraw();
}

void OSPGlutViewer::keypress(char key, const vec2i &where)
{
  switch (key) {
  case ' ':
    animationPaused = !animationPaused;
    break;
  case '=':
    animationFrameDelta = max(animationFrameDelta-0.01, 0.0001); 
    break;
  case '-':
    animationFrameDelta = min(animationFrameDelta+0.01, 1.0); 
    break;
  case 'R':
    alwaysRedraw = !alwaysRedraw;
    forceRedraw();
    break;
  case '!':
    saveScreenshot("ospglutviewer");
    break;
  case 'X':
    if (viewPort.up == vec3f(1,0,0) || viewPort.up == vec3f(-1.f,0,0)) {
      viewPort.up = - viewPort.up;
    } else {
      viewPort.up = vec3f(1,0,0);
    }
    viewPort.modified = true;
    forceRedraw();
    break;
  case 'Y':
    if (viewPort.up == vec3f(0,1,0) || viewPort.up == vec3f(0,-1.f,0)) {
      viewPort.up = - viewPort.up;
    } else {
      viewPort.up = vec3f(0,1,0);
    }
    viewPort.modified = true;
    forceRedraw();
    break;
  case 'Z':
    if (viewPort.up == vec3f(0,0,1) || viewPort.up == vec3f(0,0,-1.f)) {
      viewPort.up = - viewPort.up;
    } else {
      viewPort.up = vec3f(0,0,1);
    }
    viewPort.modified = true;
    forceRedraw();
    break;
  case 'c':
    viewPort.modified = true;//Reset accumulation
    break;
  case 'f':
    toggleFullscreen();
    break;
  case 'r':
    resetView();
    break;
  case 'p':
    printViewport();
    break;
  default:
    Glut3DWidget::keypress(key,where);
  }
}

void OSPGlutViewer::mouseButton(int32_t whichButton,
                                bool released,
                                const vec2i &pos)
{
  Glut3DWidget::mouseButton(whichButton, released, pos);
  if((currButtonState ==  (1<<GLUT_LEFT_BUTTON)) &&
     (glutGetModifiers() & GLUT_ACTIVE_SHIFT)    &&
     (manipulator == inspectCenterManipulator)) {
    vec2f normpos = vec2f(pos.x / (float)windowSize.x,
                          1.0f - pos.y / (float)windowSize.y);
    OSPPickResult pick;
    ospPick(&pick, renderer.handle(),
            osp::vec2f{normpos.x, normpos.y});
    if(pick.hit) {
      viewPort.at = ospcommon::vec3f{pick.position.x,
                                     pick.position.y,
                                     pick.position.z};
      viewPort.modified = true;
      computeFrame();
      forceRedraw();
    }
  }
}

void OSPGlutViewer::display()
{
  if (!frameBuffer.handle() || !renderer.handle() ) return;

  //{
  // note that the order of 'start' and 'end' here is
  // (intentionally) reversed: due to our asynchrounous rendering
  // you cannot place start() and end() _around_ the renderframe
  // call (which in itself will not do a lot other than triggering
  // work), but the average time between the two calls is roughly the
  // frame rate (including display overhead, of course)
  if (frameID > 0) fps.doneRender();

  // NOTE: consume a new renderer if one has been queued by another thread
  switchRenderers();

  updateAnimation(ospcommon::getSysTime()-frameTimer);
  frameTimer = ospcommon::getSysTime();

  if (resetAccum) {
    frameBuffer.clear(OSP_FB_ACCUM);
    resetAccum = false;
  }

  ++frameID;

  if (viewPort.modified) {
    Assert2(camera.handle(),"ospray camera is null");
    camera.set("pos", viewPort.from);
    auto dir = viewPort.at - viewPort.from;
    camera.set("dir", dir);
    camera.set("up", viewPort.up);
    camera.set("aspect", viewPort.aspect);
    camera.set("fovy", viewPort.openingAngle);
    camera.commit();

    viewPort.modified = false;
    frameBuffer.clear(OSP_FB_ACCUM);
  }
  fps.startRender();

  renderer.renderFrame(frameBuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

  // set the glut3d widget's frame buffer to the OSPRay frame buffer,
  // then display
  ucharFB = (uint32_t *)frameBuffer.map(OSP_FB_COLOR);
  frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
  Glut3DWidget::display();

  frameBuffer.unmap(ucharFB);

  // that pointer is no longer valid, so set it to null
  ucharFB = nullptr;

  std::string title = windowTitle;

  if (alwaysRedraw) {
    title += " (" + std::to_string((long double)fps.getFPS()) + " fps)";
    setTitle(title);
    forceRedraw();
  } else {
    setTitle(title);
  }
}

void OSPGlutViewer::switchRenderers()
{
  lock_guard<mutex> lock{rendererMutex};

  if (queuedRenderer.handle()) {
    renderer = queuedRenderer;
    queuedRenderer = nullptr;
    frameBuffer.clear(OSP_FB_ACCUM);
  }
}

void OSPGlutViewer::updateAnimation(double deltaSeconds)
{
  if (sceneModels.size() < 2)
    return;
  if (animationPaused)
    return;
  animationTimer += deltaSeconds;
  int framesSize = sceneModels.size();
  const int frameStart = (lockFirstAnimationFrame ? 1 : 0);
  if (lockFirstAnimationFrame)
    framesSize--;

  if (animationTimer > animationFrameDelta)
  {
    animationFrameId++;
    //set animation time to remainder off of delta 
    animationTimer = animationTimer - int(animationTimer/deltaSeconds)*deltaSeconds;

    size_t dataFrameId = animationFrameId%framesSize+frameStart;
    if (lockFirstAnimationFrame)
    {
      ospcommon::affine3f xfm = ospcommon::one;
      xfm = xfm*ospcommon::affine3f::translate(translate)*ospcommon::affine3f::scale(scale);
      OSPGeometry dynInst =
              ospNewInstance((OSPModel)sceneModels[dataFrameId].object(),
              (osp::affine3f&)xfm);
      ospray::cpp::Model worldModel = ospNewModel();
      ospcommon::affine3f staticXFM = ospcommon::one;
      OSPGeometry staticInst =
              ospNewInstance((OSPModel)sceneModels[0].object(),
              (osp::affine3f&)staticXFM);
      //Carson: TODO: creating new world model every frame unecessary
      worldModel.addGeometry(staticInst);
      worldModel.addGeometry(dynInst);
      worldModel.commit();
      renderer.set("model",  worldModel);
    }
    else
    {
      renderer.set("model",  sceneModels[dataFrameId]);
    }
    renderer.commit();
    resetAccumulation();
  }
}

}// namepace ospray
