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

// ours
#include "MPILoadBalancer.h"
#include "../fb/DistributedFrameBuffer.h"
// ospray
#include "ospray/fb/LocalFB.h"
#include "ospray/render/Renderer.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

namespace ospray {
  namespace mpi {

    using namespace mpicommon;

    namespace staticLoadBalancer {

      // staticLoadBalancer::Master definitions ///////////////////////////////

      float Master::renderFrame(Renderer *renderer,
                                FrameBuffer *fb,
                                const uint32 channelFlags)
      {
        UNUSED(channelFlags);
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        /* the client will do its magic here, and the distributed
           frame buffer will be writing tiles here, without us doing
           anything ourselves */
        dfb->waitUntilFinished();

        return dfb->endFrame(renderer->errorThreshold);
      }

      std::string Master::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Master";
      }

      // staticLoadBalancer::Slave definitions ////////////////////////////////

      float Slave::renderFrame(Renderer *renderer,
                               FrameBuffer *fb,
                               const uint32 channelFlags)
      {
        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        void *perFrameData = renderer->beginFrame(fb);

        const int ALLTASKS = fb->getTotalTiles();
        int NTASKS = ALLTASKS / worker.size;

        // NOTE(jda) - If all tiles do not divide evenly among all worker ranks
        //             (a.k.a. ALLTASKS / worker.size has a remainder), then
        //             some ranks will have one extra tile to do. Thus NTASKS
        //             is incremented if we are one of those ranks.
        if ((ALLTASKS % worker.size) > worker.rank)
          NTASKS++;

        tasking::parallel_for(NTASKS, [&](int taskIndex) {
          const size_t tileID = taskIndex * worker.size + worker.rank;
          const size_t numTiles_x = fb->getNumTiles().x;
          const size_t tile_y = tileID / numTiles_x;
          const size_t tile_x = tileID - tile_y*numTiles_x;
          const vec2i tileId(tile_x, tile_y);
          const int32 accumID = fb->accumID(tileId);

          if (fb->tileError(tileId) <= renderer->errorThreshold)
            return;

#define MAX_TILE_SIZE 128

#if TILE_SIZE > MAX_TILE_SIZE
          auto tilePtr = make_unique<Tile>(tileId, fb->size, accumID);
          auto &tile   = *tilePtr;
#else
          Tile __aligned(64) tile(tileId, fb->size, accumID);
#endif

          tasking::parallel_for(numJobs(renderer->spp, accumID), [&](int tid) {
            renderer->renderTile(perFrameData, tile, tid);
          });

          fb->setTile(tile);
        });

        dfb->waitUntilFinished();
        renderer->endFrame(perFrameData,channelFlags);

        return dfb->endFrame(inf); // irrelevant return value on slave, still
                                   // call to stop maml layer
      }

      std::string Slave::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Slave";
      }

      // staticLoadBalancer::Distributed definitions //////////////////////////

      float Distributed::renderFrame(Renderer *renderer,
                                     FrameBuffer *fb,
                                     const uint32 channelFlags)
      {
        auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        auto *perFrameData = renderer->beginFrame(dfb);

        tasking::parallel_for(dfb->getTotalTiles(), [&](int taskIndex) {
          const size_t numTiles_x = fb->getNumTiles().x;
          const size_t tile_y = taskIndex / numTiles_x;
          const size_t tile_x = taskIndex - tile_y*numTiles_x;
          const vec2i tileID(tile_x, tile_y);
          const int32 accumID = fb->accumID(tileID);
          const bool tileOwner = (taskIndex % numGlobalRanks()) == globalRank();

          if (dfb->tileError(tileID) <= renderer->errorThreshold)
            return;

          Tile __aligned(64) tile(tileID, dfb->size, accumID);

          const int NUM_JOBS = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
          tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
            renderer->renderTile(perFrameData, tile, tIdx);
          });

          if (tileOwner) {
            tile.generation = 0;
            tile.children = numGlobalRanks() - 1;
          } else {
            tile.generation = 1;
            tile.children = 0;
          }

          fb->setTile(tile);
        });

        dfb->waitUntilFinished();
        renderer->endFrame(nullptr, channelFlags);

        return dfb->endFrame(renderer->errorThreshold);
      }

      std::string Distributed::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Distributed";
      }

    }// ::ospray::mpi::staticLoadBalancer
  } // ::ospray::mpi
} // ::ospray
