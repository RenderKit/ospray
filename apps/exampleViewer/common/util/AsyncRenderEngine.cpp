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

#include "AsyncRenderEngine.h"

#include "sg/common/FrameBuffer.h"
#include "sg/Renderer.h"

namespace ospray {

  AsyncRenderEngine::AsyncRenderEngine(std::shared_ptr<sg::Node> sgRenderer,
                                       std::shared_ptr<sg::Node> sgRendererDW)
    : scenegraph(sgRenderer), scenegraphDW(sgRendererDW)
  {
    backgroundThread = make_unique<AsyncLoop>([&](){

      if (commitDeviceOnAsyncLoopThread) {
        auto *device = ospGetCurrentDevice();
        if (!device)
          throw std::runtime_error("could not get the current device!");
        ospDeviceCommit(device); // workaround #239
        commitDeviceOnAsyncLoopThread = false;
      }
      static sg::TimeStamp lastFTime;

      auto &sgFB = scenegraph->child("frameBuffer");

      static bool once = false;  //TODO: initial commit as timestamp cannot
                                 //      be set to 0
      static int counter = 0;
      if (sgFB.childrenLastModified() > lastFTime || !once) {
        auto &size = sgFB["size"];
        nPixels = size.valueAs<vec2i>().x * size.valueAs<vec2i>().y;
        pixelBuffers.front().resize(nPixels);
        pixelBuffers.back().resize(nPixels);
        lastFTime = sg::TimeStamp();
      }

      if (scenegraph->childrenLastModified() > lastRTime || !once) {
        double time = ospcommon::getSysTime();
        scenegraph->traverse("verify");
        double verifyTime = ospcommon::getSysTime() - time;
        time = ospcommon::getSysTime();
        scenegraph->traverse("commit");
        double commitTime = ospcommon::getSysTime() - time;

        if (scenegraphDW) {
          scenegraphDW->traverse("verify");
          scenegraphDW->traverse("commit");
        }

        lastRTime = sg::TimeStamp();
      }
      if (scenegraph->hasChild("animationcontroller"))
        scenegraph->child("animationcontroller").traverse("animate");

      if (pickPos.update()) {
        pickResult = scenegraph->nodeAs<sg::Renderer>()->pick(pickPos.ref());
      }

      fps.start();
      scenegraph->traverse("render");
      if (scenegraphDW)
        scenegraphDW->traverse("render");
      once = true;
      fps.stop();

      auto sgFBptr = sgFB.nodeAs<sg::FrameBuffer>();

      auto *srcPB = (uint32_t*)sgFBptr->map();
      auto *dstPB = (uint32_t*)pixelBuffers.back().data();

      memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));

      sgFBptr->unmap(srcPB);

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

    state = ExecState::RUNNING;
    commitDeviceOnAsyncLoopThread = true;
    backgroundThread->start();
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
