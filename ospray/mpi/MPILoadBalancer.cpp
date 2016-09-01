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
#include "mpi/MPILoadBalancer.h"
#include "render/Renderer.h"
#include "fb/LocalFB.h"
#include "mpi/DistributedFrameBuffer.h"
#include "common/tasking/parallel_for.h"

#include <algorithm>

namespace ospray {
  namespace mpi {

    // for profiling
    extern "C" void async_beginFrame();
    extern "C" void async_endFrame();

    using std::cout;
    using std::endl;

    namespace staticLoadBalancer {

      float Master::renderFrame(Renderer *renderer,
                                FrameBuffer *fb,
                                const uint32 channelFlags)
      {
        PING;
        UNUSED(channelFlags);
        async_beginFrame();
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        dfb->beginFrame();

        dfb->startNewFrame(renderer->errorThreshold);
        /* the client will do its magic here, and the distributed
           frame buffer will be writing tiles here, without us doing
           anything ourselves */
        dfb->waitUntilFinished();

        async_endFrame();

        return dfb->endFrame(renderer->errorThreshold);
      }

      std::string Master::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Master";
      }

      float Slave::renderFrame(Renderer *renderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        PING;
        async_beginFrame();

        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        // TODO WILL: Is it ok that all call begin frame? In master/worker
        // before only the master would call this.
        dfb->beginFrame();
        dfb->startNewFrame(renderer->errorThreshold);

        void *perFrameData = renderer->beginFrame(fb);

        const int ALLTASKS = fb->getTotalTiles();
        // TODO WILL: In collaborative mode rank 0 should join the worker group
        // as well so this world should actually just be worker as before
        int NTASKS = ALLTASKS / world.size;

        // NOTE(jda) - If all tiles do not divide evenly among all worker ranks
        //             (a.k.a. ALLTASKS / worker.size has a remainder), then
        //             some ranks will have one extra tile to do. Thus NTASKS
        //             is incremented if we are one of those ranks.
        if ((ALLTASKS % world.size) > world.rank)
          NTASKS++;

        // serial_for(NTASKS, [&](int taskIndex){
        parallel_for(NTASKS, [&](int taskIndex){
          const size_t tileID = taskIndex * world.size + world.rank;
          const size_t numTiles_x = fb->getNumTiles().x;
          const size_t tile_y = tileID / numTiles_x;
          const size_t tile_x = tileID - tile_y*numTiles_x;
          const vec2i tileId(tile_x, tile_y);
          const int32 accumID = fb->accumID(tileId);

          if (fb->tileError(tileId) <= renderer->errorThreshold)
            return;

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

          // serial_for(numJobs(renderer->spp, accumID), [&](int tid){
          parallel_for(numJobs(renderer->spp, accumID), [&](int tid){
            renderer->renderTile(perFrameData, tile, tid);
          });

          fb->setTile(tile);
#if TILE_SIZE>MAX_TILE_SIZE
          delete tilePtr;
#endif
        });

        std::cout << world.rank << " has finished running " << NTASKS << " tile tasks\n";

        dfb->waitUntilFinished();
        renderer->endFrame(perFrameData,channelFlags);

        async_endFrame();

        return inf; // irrelevant on slave
      }

      std::string Slave::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Slave";
      }

    }// ::ospray::mpi::staticLoadBalancer
  } // ::ospray::mpi
} // ::ospray
