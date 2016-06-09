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

OSPGlutViewer::OSPGlutViewer(const box3f &worldBounds, cpp::Model model,
                             cpp::Renderer renderer, cpp::Camera camera)
  : Glut3DWidget(Glut3DWidget::FRAMEBUFFER_NONE),
    m_model(model),
    m_fb(nullptr),
    m_renderer(renderer),
    m_camera(camera),
    m_queuedRenderer(nullptr),
    m_alwaysRedraw(true),
    m_accumID(-1),
    m_fullScreen(false)
{
  setWorldBounds(worldBounds);

  m_renderer.set("world",  m_model);
  m_renderer.set("model",  m_model);
  m_renderer.set("camera", m_camera);
  m_renderer.commit();

#if 0
  cout << "#ospDebugViewer: set world bounds " << worldBounds
       << ", motion speed " << motionSpeed << endl;
#endif

  m_resetAccum = false;
}

void OSPGlutViewer::setRenderer(OSPRenderer renderer)
{
  lock_guard<mutex> lock{m_rendererMutex};
  m_queuedRenderer = renderer;
}

void OSPGlutViewer::resetAccumulation()
{
  m_resetAccum = true;
}

void OSPGlutViewer::toggleFullscreen()
{
  m_fullScreen = !m_fullScreen;
  if(m_fullScreen) {
    glutFullScreen();
  } else {
    glutPositionWindow(0,10);
  }
}

void OSPGlutViewer::resetView()
{
  viewPort = m_viewPort;
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
  const uint32_t *p = (uint32_t*)m_fb.map(OSP_FB_COLOR);
  writePPM(basename + ".ppm", m_windowSize.x, m_windowSize.y, p);
  cout << "#ospDebugViewer: saved current frame to '" << basename << ".ppm'"
       << endl;
}

void OSPGlutViewer::reshape(const vec2i &newSize)
{
  Glut3DWidget::reshape(newSize);
  m_windowSize = newSize;
  m_fb = cpp::FrameBuffer(osp::vec2i{newSize.x, newSize.y}, OSP_FB_SRGBA,
                          OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);

  m_fb.clear(OSP_FB_ACCUM);
  m_camera.set("aspect", viewPort.aspect);
  m_camera.commit();
  viewPort.modified = true;
  forceRedraw();
}

void OSPGlutViewer::keypress(char key, const vec2i &where)
{
  switch (key) {
  case 'R':
    m_alwaysRedraw = !m_alwaysRedraw;
    forceRedraw();
    break;
  case '!':
    saveScreenshot("ospdebugviewer");
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
    ospPick(&pick, m_renderer.handle(),
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
  if (!m_fb.handle() || !m_renderer.handle()) return;

  static int frameID = 0;

  //{
  // note that the order of 'start' and 'end' here is
  // (intentionally) reversed: due to our asynchrounous rendering
  // you cannot place start() and end() _around_ the renderframe
  // call (which in itself will not do a lot other than triggering
  // work), but the average time between the two calls is roughly the
  // frame rate (including display overhead, of course)
  if (frameID > 0) m_fps.doneRender();

  // NOTE: consume a new renderer if one has been queued by another thread
  switchRenderers();

  if (m_resetAccum) {
    m_fb.clear(OSP_FB_ACCUM);
    m_resetAccum = false;
  }

  m_fps.startRender();
  //}

  ++frameID;

  if (viewPort.modified) {
    static bool once = true;
    if(once) {
      m_viewPort = viewPort;
      once = false;
    }
    Assert2(m_camera.handle(),"ospray camera is null");
    m_camera.set("pos", viewPort.from);
    auto dir = viewPort.at - viewPort.from;
    m_camera.set("dir", dir);
    m_camera.set("up", viewPort.up);
    m_camera.set("aspect", viewPort.aspect);
    m_camera.commit();
    viewPort.modified = false;
    m_accumID=0;
    m_fb.clear(OSP_FB_ACCUM);
  }

  m_renderer.renderFrame(m_fb, OSP_FB_COLOR | OSP_FB_ACCUM);
  ++m_accumID;

  // set the glut3d widget's frame buffer to the opsray frame buffer,
  // then display
  ucharFB = (uint32_t *)m_fb.map(OSP_FB_COLOR);
  frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
  Glut3DWidget::display();

  m_fb.unmap(ucharFB);

  // that pointer is no longer valid, so set it to null
  ucharFB = nullptr;

  std::string title("OSPRay Debug Viewer");

  if (m_alwaysRedraw) {
    title += " (" + std::to_string(m_fps.getFPS()) + " fps)";
    setTitle(title);
    forceRedraw();
  } else {
    setTitle(title);
  }
}

void OSPGlutViewer::switchRenderers()
{
  lock_guard<mutex> lock{m_rendererMutex};

  if (m_queuedRenderer.handle()) {
    m_renderer = m_queuedRenderer;
    m_queuedRenderer = nullptr;
    m_fb.clear(OSP_FB_ACCUM);
  }
}

}// namepace ospray
