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

#include "../common/util/AsyncRenderEngine.h"

#include "imgui3D.h"
#include "Imgui3dExport.h"

#include "common/sg/SceneGraph.h"

#include <deque>

namespace ospray {

  /*! mini scene graph viewer widget. \internal Note that all handling
    of camera is almost exactly similar to the code in volView;
    might make sense to move that into a common class! */
  class OSPRAY_IMGUI3D_INTERFACE ImGuiViewer
    : public ospray::imgui3D::ImGui3DWidget
  {
  public:

    ImGuiViewer(const std::shared_ptr<sg::Node> &scenegraph);

    ImGuiViewer(const std::shared_ptr<sg::Node> &scenegraph,
                const std::shared_ptr<sg::Node> &scenegraphDW);

    ~ImGuiViewer();

  protected:

    void mouseButton(int button, int action, int mods);
    void reshape(const ospcommon::vec2i &newSize) override;
    void keypress(char key) override;

    void resetView();
    void printViewport();
    void saveScreenshot(const std::string &basename);
    void toggleRenderingPaused();

    void display() override;

    void buildGui() override;
    void buildGUINode(std::string name, std::shared_ptr<sg::Node> node, int indent);

    void setCurrentDeviceParameter(const std::string &param, int value);

    // Data //

    double lastFrameFPS;
    double lastGUITime;
    double lastDisplayTime;
    double lastTotalTime;
    float lastVariance;

    ospcommon::vec2i windowSize;
    imgui3D::ImGui3DWidget::ViewPort originalView;

    std::shared_ptr<sg::Node> scenegraph;
    std::shared_ptr<sg::Node> scenegraphDW;

    AsyncRenderEngine renderEngine;
    std::vector<uint32_t> pixelBuffer;

    bool useDynamicLoadBalancer{false};
    int  numPreAllocatedTiles{4};
  };

}// namespace ospray
