// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "sg/common/FrameBuffer.h"

// ospImGui util
#include "ImguiUtilExport.h"

#ifdef OSPRAY_APPS_ENABLE_DENOISER
#include <OpenImageDenoise/oidn.hpp>
#endif

namespace ospray {

  using namespace ospcommon;

  enum class ExecState {STOPPED, STARTED, RUNNING, INVALID};

  class OSPRAY_IMGUI_UTIL_INTERFACE AsyncRenderEngine
  {
  public:

    AsyncRenderEngine(std::shared_ptr<sg::Frame> root);
    ~AsyncRenderEngine();

    void pick(const vec2f &screenPos);

    // Engine conrols //

    void start(int numThreads = -1);
    void stop();
    void setFrameCancelled();
#ifdef OSPRAY_APPS_ENABLE_DENOISER
    void setAsyncDenoising(const bool);
#endif

    ExecState runningState() const;

    // Output queries //

    bool   hasNewFrame() const;
    double lastFrameFps() const;
    double lastFrameFpsSmoothed() const;

    bool          hasNewPickResult();
    OSPPickResult getPickResult();

    class Framebuffer
    {
    public:
      vec2i size() const noexcept { return size_; }
      OSPFrameBufferFormat format() const noexcept { return format_; }
      void resize(const vec2i& size, const OSPFrameBufferFormat format);
      void copy(const uint8_t* src) { if (src) memcpy(buf.data(), src, bytes); }
      const uint8_t* data() const noexcept { return buf.data(); }
    private:
      vec2i size_;
      OSPFrameBufferFormat format_;
      size_t bytes;
      std::vector<uint8_t> buf;
    };

    const Framebuffer& mapFramebuffer();
    void unmapFramebuffer();

  private:

    // Helper functions //

    void validate();

    // Data //

    std::unique_ptr<AsyncLoop> backgroundThread;

    std::atomic<ExecState> state{ExecState::INVALID};
    std::atomic<bool> frameCancelled {false};

    int numOsprayThreads {-1};

    std::shared_ptr<sg::Frame> scenegraph;

    utility::TransactionalValue<vec2f> pickPos;
    utility::TransactionalValue<OSPPickResult> pickResult;
    utility::DoubleBufferedValue<Framebuffer> frameBuffers;

    std::atomic<bool> newPixels {false};

    bool commitDeviceOnAsyncLoopThread {true};

    utility::CodeTimer fps;

#ifdef OSPRAY_APPS_ENABLE_DENOISER
    class Denoiser
    {
    public:
      void init(oidn::DeviceRef &dev);
      vec2i size() const noexcept { return size_; }
      void copy(std::shared_ptr<sg::FrameBuffer>);
      void map(std::shared_ptr<sg::FrameBuffer>, vec4f* res_buf);
      void unmap(std::shared_ptr<sg::FrameBuffer>);
      void execute();
      const vec4f* result() const noexcept { return result_.data(); }
    private:
      vec2i size_;
      bool needCommit;
      std::vector<vec4f> color;
      std::vector<vec3f> normal;
      std::vector<vec3f> albedo;
      std::vector<vec4f> result_;
      const vec4f* committed_color;
      const vec3f* committed_normal;
      const vec3f* committed_albedo;
      vec4f* committed_result;
      bool committed_hdr;
      bool committed_async;
      oidn::FilterRef filter;
    };

    std::unique_ptr<AsyncLoop> denoiserThread;
    oidn::DeviceRef denoiserDevice;
    utility::DoubleBufferedValue<Denoiser> denoisers;
  public:
    double lastDenoiseFps() const;
  private:
    utility::CodeTimer denoiseFps;
    bool newBuffers {false};
    bool denoiserStop {true};
    bool asynchronousDenoising {false};
    std::condition_variable denoiserCond;
    std::mutex denoiserMutex;
#endif

  };
}// namespace ospray
