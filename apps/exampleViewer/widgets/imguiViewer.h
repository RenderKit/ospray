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

#include <ospray/ospray_cpp/Camera.h>
#include <ospray/ospray_cpp/Model.h>
#include <ospray/ospray_cpp/Renderer.h>

#include "../common/util/AsyncRenderEngine.h"

#include "imgui3D.h"
#include "Imgui3dExport.h"

#include <deque>

namespace ospray {

  /*! mini scene graph viewer widget. \internal Note that all handling
    of camera is almost exactly similar to the code in volView;
    might make sense to move that into a common class! */
  class OSPRAY_IMGUI3D_INTERFACE ImGuiViewer
    : public ospray::imgui3D::ImGui3DWidget
  {
  public:

    ImGuiViewer(const std::deque<ospcommon::box3f> &worldBounds,
                const std::deque<cpp::Model> &model,
                cpp::Renderer renderer,
                cpp::Camera camera);

    ImGuiViewer(const std::deque<ospcommon::box3f> &worldBounds,
                const std::deque<cpp::Model> &model,
                cpp::Renderer renderer,
                cpp::Renderer rendererDW,
                cpp::FrameBuffer frameBufferDW,
                cpp::Camera camera);
    ~ImGuiViewer();

    void setRenderer(OSPRenderer renderer,
                     OSPRenderer rendererDW,
                     OSPFrameBuffer frameBufferDW);
    void setScale(const ospcommon::vec3f& v )  {scale = v;}
    void setTranslation(const ospcommon::vec3f& v)  {translate = v;}
    void setLockFirstAnimationFrame(bool st) {lockFirstAnimationFrame = st;}

  protected:

    virtual void reshape(const ospcommon::vec2i &newSize) override;
    virtual void keypress(char key) override;
    virtual void mouseButton(int button, int action, int mods) override;

    void resetView();
    void printViewport();
    void saveScreenshot(const std::string &basename);
    void toggleRenderingPaused();
    // We override this so we can update the AO ray length
    void setWorldBounds(const ospcommon::box3f &worldBounds) override;

    void display() override;

    virtual void updateAnimation(double deltaSeconds);

    virtual void buildGui() override;

    // Data //

    std::deque<cpp::Model>       sceneModels;
    std::deque<ospcommon::box3f> worldBounds;
    cpp::Camera   camera;
    cpp::Renderer renderer;
    cpp::Renderer rendererDW;
    cpp::FrameBuffer frameBufferDW;

    double lastFrameFPS;

    ospcommon::vec2i windowSize;
    imgui3D::ImGui3DWidget::ViewPort originalView;

    double frameTimer;
    double animationTimer {0.};
    double animationFrameDelta {0.03};
    size_t animationFrameId {0};
    bool animationPaused {false};
    bool lockFirstAnimationFrame {false};  //use for static scene
    ospcommon::vec3f translate;
    ospcommon::vec3f scale {1.f, 1.f, 1.f};

    float aoDistance {1e20f};

    AsyncRenderEngine renderEngine;
    std::vector<uint32_t> pixelBuffer;
  };

}// namespace ospray
