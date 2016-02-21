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

// ospray
#include "ospray/mpi/MPILoadBalancer.h"
#include "ospray/render/Renderer.h"
#include "ospray/fb/LocalFB.h"
#include "ospray/mpi/DistributedFrameBuffer.h"
#include "ospray/common/parallel_for.h"

#include <algorithm>

namespace ospray {
  namespace mpi {

    // for profiling
    extern "C" void async_beginFrame();
    extern "C" void async_endFrame();

    using std::cout;
    using std::endl;

    namespace staticLoadBalancer {

      void Master::renderFrame(Renderer *tiledRenderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        async_beginFrame();
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);

        dfb->startNewFrame();
        /* the client will do its magic here, and the distributed
           frame buffer will be writing tiles here, without us doing
           anything ourselves */
        dfb->waitUntilFinished();

        async_endFrame();
      }

      std::string Master::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Master";
      }

      void Slave::RenderTask::finish() const
      {
        renderer = NULL;
        fb = NULL;
      }

      void Slave::RenderTask::run(size_t taskIndex) const
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

        const int spp = renderer->spp;
        const int blocks = (fb->accumID > 0 || spp > 0) ? 1 :
                           std::min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);
        const size_t numJobs = ((TILE_SIZE*TILE_SIZE)/
                                RENDERTILE_PIXELS_PER_JOB + blocks-1)/blocks;

        parallel_for(numJobs, [&](int taskIndex){
          renderer->renderTile(perFrameData, tile, taskIndex);
        });

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

        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        dfb->startNewFrame();

        void *perFrameData = tiledRenderer->beginFrame(fb);

        RenderTask renderTask;

        renderTask.fb = fb;
        renderTask.renderer = tiledRenderer;
        renderTask.numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        renderTask.numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
        renderTask.channelFlags = channelFlags;
        renderTask.perFrameData = perFrameData;

        const int NTASKS = renderTask.numTiles_x * renderTask.numTiles_y;
        parallel_for(NTASKS, [&](int taskIndex){renderTask.run(taskIndex);});

        dfb->waitUntilFinished();
        tiledRenderer->endFrame(perFrameData,channelFlags);

        async_endFrame();
      }

      std::string Slave::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Slave";
      }

    }

  } // ::ospray::mpi
} // ::ospray
