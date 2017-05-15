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

// std
#include <atomic>
#include <thread>
#include <vector>

// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/utility/CodeTimer.h"
#include "ospcommon/utility/TransactionalValue.h"

// ospray::cpp
#include "ospray/ospray_cpp/Renderer.h"

// ospImGui util
#include "ImguiUtilExport.h"

namespace ospray {

  enum class ExecState {STOPPED, RUNNING, INVALID};

  class OSPRAY_IMGUI_UTIL_INTERFACE AsyncRenderEngine
  {
  public:

    AsyncRenderEngine() = default;
    ~AsyncRenderEngine();

    // Properties //

    void setRenderer(cpp::Renderer renderer,
                     cpp::Renderer rendererDW,
                     cpp::FrameBuffer frameBufferDW);

    void setFbSize(const ospcommon::vec2i &size);

    // Method to say that an objects needs to be comitted before next frame //

    void scheduleObjectCommit(const cpp::ManagedObject &obj);

    // Engine conrols //

    virtual void start(int numThreads = -1);
    virtual void stop();

    ExecState runningState() const;

    // Output queries //

    bool   hasNewFrame() const;
    double lastFrameFps() const;

    const std::vector<uint32_t> &mapFramebuffer();
    void                         unmapFramebuffer();

  protected:

    // Helper functions //

    virtual void validate();
    bool checkForObjCommits();
    bool checkForFbResize();
    virtual void run();

    // Data //

    int numOsprayThreads {-1};

    std::thread backgroundThread;
    std::atomic<ExecState> state {ExecState::INVALID};

    cpp::FrameBuffer frameBuffer;
    cpp::FrameBuffer frameBufferDW;

    ospcommon::utility::TransactionalValue<cpp::Renderer>    renderer;
    ospcommon::utility::TransactionalValue<cpp::Renderer>    rendererDW;
    ospcommon::utility::TransactionalValue<ospcommon::vec2i> fbSize;

    int nPixels {0};

    int currentPB {0};
    int mappedPB  {1};
    std::mutex fbMutex;
    std::vector<uint32_t> pixelBuffer[2];

    std::mutex objMutex;
    std::vector<OSPObject> objsToCommit;

    std::atomic<bool> newPixels {false};

    ospcommon::utility::CodeTimer fps;
  };
}// namespace ospray
