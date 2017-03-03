// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "async_render_engine_sg.h"

#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace sg {

    async_render_engine_sg::async_render_engine_sg(NodeH sgRenderer)
      : scenegraph(sgRenderer)
    {
    }

    void async_render_engine_sg::run()
    {
      while (state == ExecState::RUNNING) {
        sg::RenderContext ctx;
        static sg::TimeStamp lastFTime;

        auto sgFB = scenegraph["frameBuffer"];

        if (sgFB->getChildrenLastModified() > lastFTime) {
          auto size = sgFB["size"];
          nPixels = size->getValue<vec2i>().x * size->getValue<vec2i>().y;
          pixelBuffer[0].resize(nPixels);
          pixelBuffer[1].resize(nPixels);
          lastFTime = sg::TimeStamp();
        }

        fps.startRender();

        if (scenegraph->getChildrenLastModified() > lastRTime)
        {
          scenegraph->traverse(ctx, "verify");
          scenegraph->traverse(ctx, "commit");
          lastRTime = sg::TimeStamp();
        }

        scenegraph->traverse(ctx, "render");

        fps.doneRender();
        auto sgFBptr = std::static_pointer_cast<sg::FrameBuffer>(sgFB.get());

        auto *srcPB = (uint32_t*)sgFBptr->map();
        auto *dstPB = (uint32_t*)pixelBuffer[currentPB].data();

        memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));

        sgFBptr->unmap(srcPB);

        if (fbMutex.try_lock())
        {
          std::swap(currentPB, mappedPB);
          newPixels = true;
          fbMutex.unlock();
        }
      }
    }

    void async_render_engine_sg::validate()
    {
      if (state == ExecState::INVALID)
        state = ExecState::STOPPED;
    }

  } // ::ospray::sg
} // ::ospray
