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

  AsyncRenderEngine::AsyncRenderEngine(
      std::shared_ptr<sg::Renderer> sgRenderer,
      std::shared_ptr<sg::Renderer> sgRendererDW)
    : scenegraph(sgRenderer), scenegraphDW(sgRendererDW)
  {
    auto sgFB = scenegraph->child("frameBuffer").nodeAs<sg::FrameBuffer>();

    backgroundThread = make_unique<AsyncLoop>([&, sgFB](){
      state = ExecState::RUNNING;

      if (commitDeviceOnAsyncLoopThread) {
        auto *device = ospGetCurrentDevice();
        if (!device)
          throw std::runtime_error("could not get the current device!");
        ospDeviceCommit(device); // workaround #239
        commitDeviceOnAsyncLoopThread = false;
      }
      static sg::TimeStamp lastFTime;

      static bool once = false;  //TODO: initial commit as timestamp cannot
                                 //      be set to 0
      static int counter = 0;
      if (sgFB->childrenLastModified() > lastFTime || !once) {
        auto size = sgFB->child("size").valueAs<vec2i>();
        nPixels = size.x * size.y;
        pixelBuffers.front().resize(nPixels);
        pixelBuffers.back().resize(nPixels);
        lastFTime = sg::TimeStamp();
      }

      if (scenegraph->hasChild("animationcontroller"))
        scenegraph->child("animationcontroller").animate();

      if (pickPos.update())
        pickResult = scenegraph->pick(pickPos.ref());

      fps.start();
      scenegraph->renderFrame(sgFB, OSP_FB_COLOR | OSP_FB_ACCUM, true);

      if (scenegraphDW) {
        auto dwFB =
            scenegraphDW->child("frameBuffer").nodeAs<sg::FrameBuffer>();
        scenegraphDW->renderFrame(dwFB, OSP_FB_COLOR | OSP_FB_ACCUM, true);
      }

      once = true;
      fps.stop();

      auto *srcPB = (uint32_t*)sgFB->map();
      auto *dstPB = (uint32_t*)pixelBuffers.back().data();

      memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));

      sgFB->unmap(srcPB);

      if (fbMutex.try_lock()) {
        pixelBuffers.swap();
        newPixels = true;
        fbMutex.unlock();
      }
    });
  }

  AsyncRenderEngine::~AsyncRenderEngine()
  {
    stop();
  }

  void AsyncRenderEngine::setFbSize(const ospcommon::vec2i &size)
  {
    fbSize = size;
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

  bool AsyncRenderEngine::hasNewFrame() const
  {
    return newPixels;
  }

  double AsyncRenderEngine::lastFrameFps() const
  {
    return fps.perSecondSmoothed();
  }

  float AsyncRenderEngine::getLastVariance() const
  {
    return scenegraph->nodeAs<sg::Renderer>()->getLastVariance();
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

  const std::vector<uint32_t> &AsyncRenderEngine::mapFramebuffer()
  {
    fbMutex.lock();
    newPixels = false;
    return pixelBuffers.front();
  }

  void AsyncRenderEngine::unmapFramebuffer()
  {
    fbMutex.unlock();
  }

  void AsyncRenderEngine::validate()
  {
    if (state == ExecState::INVALID)
      state = ExecState::STOPPED;
  }

}// namespace ospray
