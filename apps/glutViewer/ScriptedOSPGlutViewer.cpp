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

#include "ScriptedOSPGlutViewer.h"

// MSGViewer definitions //////////////////////////////////////////////////////

namespace ospray {

  using std::cout;
  using std::endl;
  using std::string;
  using std::lock_guard;
  using std::mutex;
  using namespace ospcommon;

  ScriptedOSPGlutViewer::ScriptedOSPGlutViewer(const box3f   &worldBounds,
                                               cpp::Model     model,
                                               cpp::Renderer  renderer,
                                               cpp::Camera    camera,
                                               std::string    scriptFileName)
    : OSPGlutViewer(worldBounds, model, renderer, camera),
      scriptHandler(model.handle(), renderer.handle(), camera.handle(), this),
      frameID(0)
  {
    if (!scriptFileName.empty())
      scriptHandler.runScriptFromFile(scriptFileName);
  }

  int ScriptedOSPGlutViewer::getFrameID() const {
    return frameID.load();
  }

  void ScriptedOSPGlutViewer::display() {
    if (!frameBuffer.handle() || !renderer.handle()) return;

    // We need to synchronize with the scripting engine so we don't
    // get our scene data trampled on if scripting is running.
    std::lock_guard<std::mutex> lock(scriptHandler.scriptMutex);

    //{
    // note that the order of 'start' and 'end' here is
    // (intentionally) reversed: due to our asynchrounous rendering
    // you cannot place start() and end() _around_ the renderframe
    // call (which in itself will not do a lot other than triggering
    // work), but the average time between the two calls is roughly the
    // frame rate (including display overhead, of course)
    if (frameID.load() > 0) fps.doneRender();

    // NOTE: consume a new renderer if one has been queued by another thread
    switchRenderers();

    if (resetAccum) {
      frameBuffer.clear(OSP_FB_ACCUM);
      resetAccum = false;
    }

    fps.startRender();
    //}

    ++frameID;

    if (viewPort.modified) {
      static bool once = true;
      if(once) {
        glutViewPort = viewPort;
        once = false;
      }
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

    renderer.renderFrame(frameBuffer, OSP_FB_COLOR | OSP_FB_ACCUM);

    // set the glut3d widget's frame buffer to the opsray frame buffer,
    // then display
    ucharFB = (uint32_t *)frameBuffer.map(OSP_FB_COLOR);
    frameBufferMode = Glut3DWidget::FRAMEBUFFER_UCHAR;
    Glut3DWidget::display();

    frameBuffer.unmap(ucharFB);

    // that pointer is no longer valid, so set it to null
    ucharFB = nullptr;

    std::string title("OSPRay GLUT Viewer");

    if (alwaysRedraw) {
      title += " (" + std::to_string((long double)fps.getFPS()) + " fps)";
      setTitle(title);
      forceRedraw();
    } else {
      setTitle(title);
    }
  }

  void ScriptedOSPGlutViewer::keypress(char key, const vec2i &where)
  {
    switch (key) {
    case ':':
      if (!scriptHandler.running()) {
        scriptHandler.start();
      }
      break;
    case 27 /*ESC*/:
    case 'q':
    case 'Q':
      if (scriptHandler.running()) {
        std::cout << "Please exit command mode before quitting"
          << " to avoid messing up your terminal\n";
        break;
      } else {
        std::exit(0);
      }
    default:
      OSPGlutViewer::keypress(key,where);
    }
  }

}// namepace ospray
