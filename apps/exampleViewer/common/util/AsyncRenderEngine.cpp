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

#include "AsyncRenderEngine.h"

#include "sg/common/FrameBuffer.h"
#include "sg/Renderer.h"

namespace ospray {

  AsyncRenderEngine::AsyncRenderEngine(std::shared_ptr<sg::Frame> root)
    : scenegraph(root)
  {
    auto renderer = scenegraph->child("renderer").nodeAs<sg::Renderer>();

    backgroundThread = make_unique<AsyncLoop>([&, renderer](){
      state = ExecState::RUNNING;

      if (commitDeviceOnAsyncLoopThread) {
        auto *device = ospGetCurrentDevice();
        if (!device)
          throw std::runtime_error("could not get the current device!");
        ospDeviceCommit(device); // workaround #239
        commitDeviceOnAsyncLoopThread = false;
      }

      if (renderer->hasChild("animationcontroller"))
        renderer->child("animationcontroller").animate();

      if (pickPos.update())
        pickResult = scenegraph->pick(pickPos.ref());

      fps.start();
      auto sgFB = scenegraph->renderFrame();
      fps.stop();

      if (frameCancelled.exchange(false))
        return; // actually a continue

      if (!sgFB) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        return; // actually a continue
      }

      // NOTE(jda) - Spin here until the consumer has had the chance to update
      //             to the latest frame.
      while(state == ExecState::RUNNING && newPixels == true);

      frameBuffers.back().resize(sgFB->size(), sgFB->format());
      auto srcPB = (uint8_t*)sgFB->map();
      frameBuffers.back().copy(srcPB);
      sgFB->unmap(srcPB);

      newPixels = true;
    }, AsyncLoop::LaunchMethod::THREAD);
  }

  AsyncRenderEngine::~AsyncRenderEngine()
  {
    stop();
  }

  void AsyncRenderEngine::start(int numOsprayThreads)
  {
    if (state == ExecState::RUNNING)
      return;

    validate();

    if (state == ExecState::INVALID)
      throw std::runtime_error("Can't start the engine in an invalid state!");

    if (numOsprayThreads > 0) {
      auto device = ospGetCurrentDevice();
      if(device == nullptr)
        throw std::runtime_error("Can't get current device!");

      ospDeviceSet1i(device, "numThreads", numOsprayThreads);
    }

    state = ExecState::STARTED;
    commitDeviceOnAsyncLoopThread = true;

    // NOTE(jda) - This whole loop is because I haven't found a way to get
    //             AsyncLoop to robustly start. A very small % of the time,
    //             calling start() won't actually wake the thread which the
    //             AsyncLoop is running the loop on, causing the render loop to
    //             never actually run...I hope someone can find a better
    //             solution!
    while (state != ExecState::RUNNING) {
      backgroundThread->stop();
      backgroundThread->start();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }

  void AsyncRenderEngine::stop()
  {
    if (state != ExecState::RUNNING)
      return;

    state = ExecState::STOPPED;
    backgroundThread->stop();
  }

  ExecState AsyncRenderEngine::runningState() const
  {
    return state;
  }

  void AsyncRenderEngine::setFrameCancelled()
  {
    frameCancelled = true;
  }

  bool AsyncRenderEngine::hasNewFrame() const
  {
    return newPixels;
  }

  double AsyncRenderEngine::lastFrameFps() const
  {
    return fps.perSecond();
  }

  double AsyncRenderEngine::lastFrameFpsSmoothed() const
  {
    return fps.perSecondSmoothed();
  }

  void AsyncRenderEngine::pick(const vec2f &screenPos)
  {
    pickPos = screenPos;
  }

  bool AsyncRenderEngine::hasNewPickResult()
  {
    return pickResult.update();
  }

  OSPPickResult AsyncRenderEngine::getPickResult()
  {
    return pickResult.get();
  }

  const AsyncRenderEngine::Framebuffer &AsyncRenderEngine::mapFramebuffer()
  {
    if (newPixels) {
      frameBuffers.swap();
      newPixels = false;
    }
    return frameBuffers.front();
  }

  void AsyncRenderEngine::unmapFramebuffer()
  {
    // no-op
  }

  void AsyncRenderEngine::validate()
  {
    if (state == ExecState::INVALID)
      state = ExecState::STOPPED;
  }

  // Framebuffer impl
  void AsyncRenderEngine::Framebuffer::resize(const vec2i& size,
      const OSPFrameBufferFormat format)
  {
    format_ = format;
    size_ = size;
    bytes = size.x * size.y;
    switch (format) {
      default: /* fallthrough */
      case OSP_FB_NONE:
        bytes = 0;
        size_ = vec2i(0);
        break;
      case OSP_FB_RGBA8: /* fallthrough */
      case OSP_FB_SRGBA:
        bytes *= sizeof(uint32_t);
        break;
      case OSP_FB_RGBA32F:
        bytes *= 4*sizeof(float);
        break;
    }
    buf.reserve(bytes);
  }

}// namespace ospray
