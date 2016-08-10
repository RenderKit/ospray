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

#include "GlutViewerScriptHandler.h"
#include "ScriptedOSPGlutViewer.h"

#include <iostream>
using std::endl;

namespace ospray {

  GlutViewerScriptHandler::GlutViewerScriptHandler(OSPModel    model,
                                                   OSPRenderer renderer,
                                                   OSPCamera   camera,
                                                   ScriptedOSPGlutViewer  *viewer)
    : OSPRayScriptHandler(model, renderer, camera),
      viewer(viewer)
  {
    registerScriptFunctions();

    std::stringstream ss;

    ss << "Viewer functions available:" << endl << endl;
    ss << "setRenderer(renderer) --> set the renderer in the viewer" << endl;
    ss << "refresh()             --> reset the accumulation buffer" << endl;
    ss << "toggleFullScreen()    --> toggle fullscreen mode" << endl;
    ss << "resetView()           --> reset camera view" << endl;
    ss << "printViewport()       --> print view params in the console" << endl;
    ss << "renderFrame(n_frames) --> release the script lock and render 'n_frames' frames,\n"
       << "                          then return to running the script." << endl;
    ss << "setWorldBounds(bbox)  --> set the world bounds to the specified box3f,\n"
       << "                          repositioning the camera." << endl;
    ss << "screenshot(filename)  --> save a screenshot (adds '.ppm')" << endl;

    helpText += ss.str();
  }

  void GlutViewerScriptHandler::registerScriptFunctions()
  {
    auto &chai = this->scriptEngine();

    // setRenderer()
    auto setRenderer = [&](ospray::cpp::Renderer &r) {
      viewer->setRenderer((OSPRenderer)r.handle());
    };

    // refresh()
    auto refresh = [&]() {
      viewer->resetAccumulation();
    };

    // toggleFullscreen()
    auto toggleFullscreen = [&]() {
      viewer->toggleFullscreen();
    };

    // resetView()
    auto resetView = [&]() {
      viewer->resetView();
    };

    // printViewport()
    auto printViewport = [&]() {
      viewer->printViewport();
    };

    // renderFrame()
    auto renderFrame = [&](const int n_frames) {
      // Temporarily unlock the mutex and wait for the display
      // loop to acquire it and render and wait til a n frames have finished
      const int startFrame = viewer->getFrameID();
      lock.unlock();
      // Wait for n_frames to be rendered
      while (startFrame + n_frames > viewer->getFrameID());
      lock.lock();
    };
    auto renderOneFrame = [&]() {
      // Temporarily unlock the mutex and wait for the display
      // loop to acquire it and render and wait til a n frames have finished
      const int startFrame = viewer->getFrameID();
      lock.unlock();
      // Wait for n_frames to be rendered
      while (startFrame + 1 > viewer->getFrameID());
      lock.lock();
    };

    // setWorldBounds
    auto setWorldBounds = [&](const ospcommon::box3f &box) {
      viewer->setWorldBounds(box);
    };

    // screenshot()
    auto screenshot = [&](const std::string &name) {
      viewer->saveScreenshot(name);
    };

    chai.add(chaiscript::fun(setRenderer),      "setRenderer"     );
    chai.add(chaiscript::fun(refresh),          "refresh"         );
    chai.add(chaiscript::fun(toggleFullscreen), "toggleFullscreen");
    chai.add(chaiscript::fun(resetView),        "resetView"       );
    chai.add(chaiscript::fun(printViewport),    "printViewport"   );
    chai.add(chaiscript::fun(renderFrame),      "renderFrame"     );
    chai.add(chaiscript::fun(renderOneFrame),   "renderFrame"     );
    chai.add(chaiscript::fun(setWorldBounds),   "setWorldBounds"  );
    chai.add(chaiscript::fun(screenshot),       "screenshot"      );
  }

}// namespace ospray
