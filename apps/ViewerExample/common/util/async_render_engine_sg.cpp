#include "async_render_engine_sg.h"

#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace sg {

  void async_render_engine_sg::validate()
  {
    if (state == ExecState::INVALID)
    {
      state = ExecState::STOPPED;
    }
  }

  void async_render_engine_sg::start(int numThreads)
  {
    paused = false;
    if (state == ExecState::RUNNING)
      return;

    numOsprayThreads = numThreads;

    validate();

    if (state == ExecState::INVALID)
      throw std::runtime_error("Can't start the engine in an invalid state!");

    if (!runningThreads)
    {
      backgroundThread = std::thread(&async_render_engine_sg::run, this);
      runningThreads = numThreads;
    }
    state = ExecState::RUNNING;
  }

  void async_render_engine_sg::stop()
  {
    if (state != ExecState::RUNNING)
      return;

    // state = ExecState::STOPPED;
    paused = true;
    // if (backgroundThread.joinable())
      // backgroundThread.join();
  }

  void async_render_engine_sg::run()
  {
    while (state == ExecState::RUNNING) {
      if (paused)
        continue;
      // bool resetAccum = false;
      // resetAccum |= renderer.update();
      // resetAccum |= checkForFbResize();
      // resetAccum |= checkForObjCommits();

      sg::RenderContext ctx;
      static sg::TimeStamp lastFTime = 0;
      if (scenegraph["frameBuffer"]->getChildrenLastModified() > lastFTime )
      {
        nPixels = scenegraph["frameBuffer"]["size"]->getValue<vec2i>().x *
        scenegraph["frameBuffer"]["size"]->getValue<vec2i>().y;
        pixelBuffer[0].resize(nPixels);
        pixelBuffer[1].resize(nPixels);
        lastFTime = sg::TimeStamp::now();
      }
      fps.startRender();
      bool modified = (scenegraph->getChildrenLastModified() > lastRTime);
      if (modified)
      {
        scenegraph->traverse(ctx, "verify");
        scenegraph->traverse(ctx, "commit");
        lastRTime = sg::TimeStamp::now();
      }


      // if (modified)
        // frameBuffer.clear(OSP_FB_ACCUM);

        scenegraph->traverse(ctx, "render");
      // ospRenderFrame((OSPFrameBuffer)scenegraph["frameBuffer"]->getValue<OSPObject>(),
      //          (OSPRenderer)scenegraph->getValue<OSPObject>(),
      //          OSP_FB_COLOR | OSP_FB_ACCUM);

      fps.doneRender();
      std::shared_ptr<sg::FrameBuffer> sgFB = 
        std::static_pointer_cast<sg::FrameBuffer>(scenegraph["frameBuffer"].get());

      auto *srcPB = (uint32_t*) sgFB->map();
      // auto *srcPB = (uint32_t*)frameBuffer.map(OSP_FB_COLOR);
      auto *dstPB = (uint32_t*)pixelBuffer[currentPB].data();

      memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));

      // frameBuffer.unmap(srcPB);

      sgFB->unmap((unsigned char*)srcPB);

      if (fbMutex.try_lock())
      {
        std::swap(currentPB, mappedPB);
        newPixels = true;
        fbMutex.unlock();
      }
    }
  }

  }
}