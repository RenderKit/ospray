// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "../common/util/AsyncRenderEngine.h"

#include "imgui3D.h"
#include "transferFunction.h"

#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"

namespace ospray {

  /*! mini scene graph viewer widget. \internal Note that all handling
    of camera is almost exactly similar to the code in volView;
    might make sense to move that into a common class! */
  class OSPRAY_IMGUI3D_INTERFACE ImGuiViewer
    : public ospray::imgui3D::ImGui3DWidget
  {
  public:

    ImGuiViewer(const std::shared_ptr<sg::Frame> &scenegraph);

    ~ImGuiViewer() override;

    void setInitialSearchBoxText(const std::string &text);
    void setColorMap(std::string name);

    template <typename CALLBACK_T>
    void addCustomUICallback(const std::string &name, CALLBACK_T &&f);

    void startAsyncRendering() override;

    void setViewportToSgCamera();
    void setDefaultViewportToCurrent();

  protected:

    enum PickMode { PICK_CAMERA, PICK_NODE };

    void mouseButton(int button, int action, int mods) override;
    void reshape(const ospcommon::vec2i &newSize) override;
    void keypress(char key) override;

    void resetView();
    void resetDefaultView();
    void printViewport();
    void toggleRenderingPaused();

    void display() override;

    void buildGui() override;

    void guiMenu();
    void guiMenuApp();
    void guiMenuView();
    void guiMenuMPI();

    void guiRenderStats();
    void guiRenderCustomWidgets();
    void guiTransferFunction();
    void guiFindNode();

    void guiSearchSGNodes();

    void setCurrentDeviceParameter(const std::string &param, int value);

    // Data //

    double lastGUITime;
    double lastDisplayTime;
    double lastTotalTime;

    imgui3D::ImGui3DWidget::ViewPort originalView;
    bool saveScreenshot {false}; // write next mapped framebuffer to disk
    bool cancelFrameOnInteraction {true};

    float frameProgress {0.f};
    std::atomic<bool> cancelRendering {false};
    int progressCallback(const float progress);
    static int progressCallbackWrapper(void * ptr, const float progress);

    std::shared_ptr<sg::Frame> scenegraph;
    std::shared_ptr<sg::Renderer> renderer;

    std::string nodeNameForSearch;
    std::vector<std::shared_ptr<sg::Node>> collectedNodesFromSearch;

    using UICallback = std::function<void(sg::Frame &)>;
    using NamedUICallback = std::pair<std::string, UICallback>;

    std::vector<NamedUICallback> customPanes;

    AsyncRenderEngine renderEngine;

    bool useDynamicLoadBalancer{false};
    int  numPreAllocatedTiles{4};

    PickMode lastPickQueryType {PICK_CAMERA};

    imgui3D::TransferFunction transferFunctionWidget;
    int transferFunctionSelection{0};
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename CALLBACK_T>
  inline void
  ImGuiViewer::addCustomUICallback(const std::string &name, CALLBACK_T &&f)
  {
    // TODO: static_assert the signature of CALLBACK_T::operator()

    customPanes.push_back({name, f});
  }

}// namespace ospray
