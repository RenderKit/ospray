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
#include "ospray/common/tasking/parallel_for.h"

#include <algorithm>

namespace ospray {
  namespace mpi {

    // for profiling
    extern "C" void async_beginFrame();
    extern "C" void async_endFrame();

    using std::cout;
    using std::endl;

    namespace staticLoadBalancer {

      float Master::renderFrame(Renderer *tiledRenderer,
                                FrameBuffer *fb,
                                const uint32 channelFlags)
      {
        async_beginFrame();
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        dfb->startNewFrame();
        /* the client will do its magic here, and the distributed
           frame buffer will be writing tiles here, without us doing
           anything ourselves */
        dfb->waitUntilFinished();

        async_endFrame();

        return dfb->endFrame(0.f);
      }

      std::string Master::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Master";
      }

      float Slave::renderFrame(Renderer *tiledRenderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        async_beginFrame();

        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        dfb->startNewFrame();

        void *perFrameData = tiledRenderer->beginFrame(fb);

        int numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
        int numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);

        const int NTASKS = numTiles_x * numTiles_y;
        // serial_for(NTASKS, [&](int taskIndex){
        parallel_for(NTASKS, [&](int taskIndex){
          const size_t tileID = taskIndex;
          if ((tileID % worker.size) != worker.rank) return;

          const size_t tile_y = tileID / numTiles_x;
          const size_t tile_x = tileID - tile_y*numTiles_x;
          const vec2i tileId(tile_x, tile_y);
          const int32 accumID = fb->accumID(tileID);

#ifdef __MIC__
#  define MAX_TILE_SIZE 32
#else
#  define MAX_TILE_SIZE 128
#endif

#if TILE_SIZE>MAX_TILE_SIZE
          Tile *tilePtr = new Tile(tileId, fb->size, accumID);
          Tile &tile = *tilePtr;
#else
          Tile __aligned(64) tile(tileId, fb->size, accumID);
#endif

          // serial_for(numJobs(tiledRenderer->spp, accumID), [&](int taskIndex){
          parallel_for(numJobs(tiledRenderer->spp, accumID), [&](int taskIndex){
            tiledRenderer->renderTile(perFrameData, tile, taskIndex);
          });

          fb->setTile(tile);
#if TILE_SIZE>MAX_TILE_SIZE
          delete tilePtr;
#endif
        });

        dfb->waitUntilFinished();
        tiledRenderer->endFrame(perFrameData,channelFlags);

        async_endFrame();

        return dfb->endFrame(0.f);
      }

      std::string Slave::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Slave";
      }

    }// ::ospray::mpi::staticLoadBalancer
  } // ::ospray::mpi
} // ::ospray
