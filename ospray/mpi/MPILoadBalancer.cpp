// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// ospray
#include "ospray/mpi/MPILoadBalancer.h"
#include "ospray/render/Renderer.h"
#include "ospray/fb/LocalFB.h"
#include "ospray/mpi/DistributedFrameBuffer.h"

//#define BARRIER_AT_END_OF_FRAME 1

namespace ospray {
  namespace mpi {

    // for profiling
    extern "C" void async_beginFrame();
    extern "C" void async_endFrame();

    using std::cout; 
    using std::endl;

    namespace staticLoadBalancer {

      Master::Master() {
      }

      void Master::renderFrame(Renderer *tiledRenderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        async_beginFrame();
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);

        // double before = getSysTime();
        dfb->startNewFrame();
        /* the client will do its magic here, and the distributed
           frame buffer will be writing tiles here, without us doing
           anything ourselves */
        dfb->waitUntilFinished();
        // double after = getSysTime();
        // float T = after - before;
        // printf("master: render time %f, theofps %f\n",T,1.f/T);

        async_endFrame();
        // #if BARRIER_AT_END_OF_FRAME
        //         MPI_Barrier(MPI_COMM_WORLD);
        // #endif
      }

      void Slave::RenderTask::finish()
      {
        renderer = NULL;
        fb = NULL;
      }


      Slave::Slave() {
      }


      void Slave::RenderTask::run(size_t taskIndex) 
      {
        const size_t tileID = taskIndex;
        if ((tileID % worker.size) != worker.rank) return;

#if TILE_SIZE>128
        Tile *tilePtr = new Tile;
        Tile &tile = *tilePtr;
#else
        Tile __aligned(64) tile;
#endif
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID - tile_y*numTiles_x;
        tile.region.lower.x = tile_x * TILE_SIZE;
        tile.region.lower.y = tile_y * TILE_SIZE;
        tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
        tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
        tile.fbSize = fb->size;
        tile.rcp_fbSize = rcp(vec2f(fb->size));
        tile.generation = 0;
        tile.children = 0;

        renderer->renderTile(perFrameData,tile);

        fb->setTile(tile);
#if TILE_SIZE>128
        delete tilePtr;
#endif
      }
      
      void Slave::renderFrame(Renderer *tiledRenderer, 
                              FrameBuffer *fb,
                              const uint32 channelFlags
                              )
      {

        async_beginFrame();

        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
        dfb->startNewFrame();

        void *perFrameData = tiledRenderer->beginFrame(fb);

        Ref<RenderTask> renderTask = new RenderTask;
        renderTask->fb = fb;
        renderTask->renderer = tiledRenderer;
        renderTask->numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        renderTask->numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
        renderTask->channelFlags = channelFlags;
        renderTask->perFrameData = perFrameData;

        /*! iw: using a local sync event for now; "in theory" we should be
          able to attach something like a sync event to the frame
          buffer, just trigger the task here, and let somebody else sync
          on the framebuffer once it is needed; alas, I'm currently
          running into some issues with the embree taks system when
          trying to do so, and thus am reverting to this
          fully-synchronous version for now */
        renderTask->schedule(renderTask->numTiles_x*renderTask->numTiles_y);
        renderTask->wait();

        // double t0wait = getSysTime();
        dfb->waitUntilFinished();
        tiledRenderer->endFrame(perFrameData,channelFlags);
        // double t1wait = getSysTime();
        // printf("rank %i t_wait at end %f\n",mpi::world.rank,float(t1wait-t0wait));

        async_endFrame();
      }
    }

  } // ::ospray::mpi
} // ::ospray
