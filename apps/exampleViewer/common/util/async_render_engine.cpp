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

#include "async_render_engine.h"

namespace ospray {

  async_render_engine::~async_render_engine()
  {
    stop();
  }

  void async_render_engine::setRenderer(cpp::Renderer renderer)
  {
    this->renderer = renderer;
  }

  void async_render_engine::setFbSize(const ospcommon::vec2i &size)
  {
    fbSize = size;
  }

  void async_render_engine::scheduleObjectCommit(const cpp::ManagedObject &obj)
  {
    std::lock_guard<std::mutex> lock{objMutex};
    objsToCommit.push_back(obj.object()); 
  }

  void async_render_engine::start(int numThreads)
  {
    if (state == ExecState::RUNNING)
      return;

    numOsprayThreads = numThreads;

    validate();

    if (state == ExecState::INVALID)
      throw std::runtime_error("Can't start the engine in an invalid state!");

    state = ExecState::RUNNING;
    backgroundThread = std::thread([&](){ run(); });
  }

  void async_render_engine::stop()
  {
    if (state != ExecState::RUNNING)
      return;

    state = ExecState::STOPPED;
    if (backgroundThread.joinable())
      backgroundThread.join();
  }

  ExecState async_render_engine::runningState() const
  {
    return state;
  }

  bool async_render_engine::hasNewFrame() const
  {
    return newPixels;
  }

  double async_render_engine::lastFrameFps() const
  {
    return fps.getFPS();
  }

  const std::vector<uint32_t> &async_render_engine::mapFramebuffer()
  {
    fbMutex.lock();
    newPixels = false;
    return pixelBuffer[mappedPB];
  }

  void async_render_engine::unmapFramebuffer()
  {
    fbMutex.unlock();
  }

  void async_render_engine::validate()
  {
    if (state == ExecState::INVALID)
    {
      renderer.update();
      state = renderer.ref().handle() ? ExecState::STOPPED : ExecState::INVALID;
    }
  }

  bool async_render_engine::checkForObjCommits()
  {
    bool commitOccurred = false;

    if (!objsToCommit.empty()) {
      std::lock_guard<std::mutex> lock{objMutex};

      for (auto obj : objsToCommit)
        ospCommit(obj);

      objsToCommit.clear();
      commitOccurred = true;
    }

    return commitOccurred;
  }

  bool async_render_engine::checkForFbResize()
  {
    bool changed = fbSize.update();

    if (changed) {
      auto &size  = fbSize.ref();
      frameBuffer = cpp::FrameBuffer(osp::vec2i{size.x, size.y}, OSP_FB_SRGBA,
                                     OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM);

      nPixels = size.x * size.y;
      pixelBuffer[0].resize(nPixels);
      pixelBuffer[1].resize(nPixels);
    }

    return changed;
  }

  void async_render_engine::run()
  {
    auto device = ospGetCurrentDevice();
    if (numOsprayThreads > 0)
      ospDeviceSet1i(device, "numThreads", numOsprayThreads);
    ospDeviceCommit(device);

    while (state == ExecState::RUNNING) {
      bool resetAccum = false;
      resetAccum |= renderer.update();
      resetAccum |= checkForFbResize();
      resetAccum |= checkForObjCommits();

      if (resetAccum) {
        frameBuffer.clear(OSP_FB_ACCUM);
      }

      fps.startRender();
      renderer.ref().renderFrame(frameBuffer, OSP_FB_COLOR | OSP_FB_ACCUM);
      fps.doneRender();

      auto *srcPB = (uint32_t*)frameBuffer.map(OSP_FB_COLOR);
      auto *dstPB = (uint32_t*)pixelBuffer[currentPB].data();

      memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));

      frameBuffer.unmap(srcPB);

      if (fbMutex.try_lock())
      {
        std::swap(currentPB, mappedPB);
        newPixels = true;
        fbMutex.unlock();
      }
    }
  }

}// namespace ospray
