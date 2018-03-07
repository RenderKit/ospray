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

// std
#include <atomic>
#include <vector>

// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/AsyncLoop.h"
#include "ospcommon/utility/CodeTimer.h"
#include "ospcommon/utility/DoubleBufferedValue.h"
#include "ospcommon/utility/TransactionalValue.h"

#include "sg/Renderer.h"

// ospImGui util
#include "ImguiUtilExport.h"

namespace ospray {

  using namespace ospcommon;

  enum class ExecState {STOPPED, STARTED, RUNNING, INVALID};

  class OSPRAY_IMGUI_UTIL_INTERFACE AsyncRenderEngine
  {
  public:

    AsyncRenderEngine(std::shared_ptr<sg::Renderer> sgRenderer,
                      std::shared_ptr<sg::Renderer> sgRendererDW);
    ~AsyncRenderEngine();

    // Properties //

    void setFbSize(const vec2i &size);

    // Method to say that an objects needs to be comitted before next frame //

    void pick(const vec2f &screenPos);

    // Engine conrols //

    void start(int numThreads = -1);
    void stop();

    ExecState runningState() const;

    // Output queries //

    bool   hasNewFrame() const;
    double lastFrameFps() const;
    float  getLastVariance() const;

    bool          hasNewPickResult();
    OSPPickResult getPickResult();

    const std::vector<uint32_t> &mapFramebuffer();
    void                         unmapFramebuffer();

    private:

    // Helper functions //

    void validate();

    // Data //

    std::unique_ptr<AsyncLoop> backgroundThread;

    std::atomic<ExecState> state{ExecState::INVALID};

    int numOsprayThreads {-1};

    std::shared_ptr<sg::Renderer> scenegraph;
    std::shared_ptr<sg::Renderer> scenegraphDW;

    utility::TransactionalValue<vec2i> fbSize;
    utility::TransactionalValue<vec2f> pickPos;
    utility::TransactionalValue<OSPPickResult> pickResult;

    int nPixels {0};

    std::mutex fbMutex;
    utility::DoubleBufferedValue<std::vector<uint32_t>> pixelBuffers;

    std::atomic<bool> newPixels {false};

    bool commitDeviceOnAsyncLoopThread {true};

    utility::CodeTimer fps;
  };
}// namespace ospray
