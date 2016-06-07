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

#pragma once

#include <atomic>
#include <mutex>

// viewer widget
#include "common/widgets/glut3D.h"
// mini scene graph for loading the model
#include "common/miniSG/miniSG.h"

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/Renderer.h>

namespace ospray {

/*! mini scene graph viewer widget. \internal Note that all handling
  of camera is almost exactly similar to the code in volView;
  might make sense to move that into a common class! */
class OSPGlutViewer : public ospray::glut3D::Glut3DWidget
{
public:

  OSPGlutViewer(const ospcommon::box3f &worldBounds, cpp::Model model,
                cpp::Renderer renderer, cpp::Camera camera);

  void setRenderer(OSPRenderer renderer);
  void resetAccumulation();
  void toggleFullscreen();
  void resetView();
  void printViewport();
  void saveScreenshot(const std::string &basename);

protected:

  virtual void reshape(const ospcommon::vec2i &newSize) override;
  virtual void keypress(char key, const ospcommon::vec2i &where) override;
  virtual void mouseButton(int32_t whichButton, bool released,
                           const ospcommon::vec2i &pos) override;

private:

  void display() override;

  void switchRenderers();

  // Data //

  cpp::Model       m_model;
  cpp::FrameBuffer m_fb;
  cpp::Renderer    m_renderer;
  cpp::Camera      m_camera;

  ospray::glut3D::FPSCounter m_fps;

  std::mutex m_rendererMutex;
  cpp::Renderer m_queuedRenderer;

  bool m_alwaysRedraw;

  ospcommon::vec2i m_windowSize;
  int m_accumID;
  bool m_fullScreen;
  glut3D::Glut3DWidget::ViewPort m_viewPort;

  std::atomic<bool> m_resetAccum;
};

}// namespace ospray
