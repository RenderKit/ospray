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

#include "AsyncRenderEngineSg.h"

#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace sg {

    AsyncRenderEngineSg::AsyncRenderEngineSg(const std::shared_ptr<Node> &sgRenderer,
                                             const std::shared_ptr<Node> &sgRendererDW)
      : scenegraph(sgRenderer),
        scenegraphDW(sgRendererDW)
    {
    }

    void AsyncRenderEngineSg::run()
    {
      while (state == ExecState::RUNNING) {
        static sg::TimeStamp lastFTime;

        auto &sgFB = scenegraph->child("frameBuffer");

        if (sgFB.childrenLastModified() > lastFTime) {
          auto &size = sgFB["size"];
          nPixels = size.valueAs<vec2i>().x * size.valueAs<vec2i>().y;
          pixelBuffer[0].resize(nPixels);
          pixelBuffer[1].resize(nPixels);
          lastFTime = sg::TimeStamp();
        }


        if (scenegraph->childrenLastModified() > lastRTime) {
          static int once = 0;
          scenegraph->traverse("verify");
          scenegraph->traverse("commit");

          if (scenegraphDW) {
            scenegraphDW->traverse("verify");
            scenegraphDW->traverse("commit");
          }

          lastRTime = sg::TimeStamp();
        }

        fps.startRender();
        scenegraph->traverse("render");
        if (scenegraphDW) 
          scenegraphDW->traverse("render");
        
        fps.doneRender();
        auto sgFBptr =
            std::static_pointer_cast<sg::FrameBuffer>(sgFB.shared_from_this());

        auto *srcPB = (uint32_t*)sgFBptr->map();
        auto *dstPB = (uint32_t*)pixelBuffer[currentPB].data();

        memcpy(dstPB, srcPB, nPixels*sizeof(uint32_t));
        
        sgFBptr->unmap(srcPB);

        if (fbMutex.try_lock()) {
          std::swap(currentPB, mappedPB);
          newPixels = true;
          fbMutex.unlock();
        }
      }
    }

    void AsyncRenderEngineSg::validate()
    {
      if (state == ExecState::INVALID)
        state = ExecState::STOPPED;
    }

  } // ::ospray::sg
} // ::ospray
