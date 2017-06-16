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
#include "ospray/render/Renderer.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/tasking/schedule.h"
// std
#include <algorithm>

namespace ospray {
  namespace mpi {

    using namespace mpicommon;

    namespace staticLoadBalancer {

      // staticLoadBalancer::Master definitions ///////////////////////////////

      float Master::renderFrame(Renderer *renderer,
                                FrameBuffer *fb,
                                const uint32 /*channelFlags*/)
      {
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

    namespace dynamicLoadBalancer {

      // dynamicLoadBalancer::Master definitions ///////////////////////////////

      Master::Master()
      {
        mpi::messaging::registerMessageListener(myId, this);
        preferredTiles.resize(worker.size);
        workerNotified.resize(worker.size);

        auto OSPRAY_PREALLOCATED_TILES = getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");
        numPreAllocated = OSPRAY_PREALLOCATED_TILES.first ? OSPRAY_PREALLOCATED_TILES.second : 4;
        PRINT(numPreAllocated);
      }

      void Master::incoming(const std::shared_ptr<mpicommon::Message> &msg)
      {
        const int requester = *(int*)msg->data;
        int w = workerRankFromGlobalRank(requester);
        if (workerNotified[w])
          return;

        ssize_t tileIndex = -1;
        if (preferredTiles[w] > numPreAllocated)
          tileIndex = --preferredTiles[w] * worker.size + w;
        else { // look for largest non-empty queue
          int ms = numPreAllocated;
          int mi = -1;
          for (w = 0; w < worker.size; w++)
            if (preferredTiles[w] > ms) {
              ms = preferredTiles[w];
              mi = w;
            }
          if (mi > -1)
            tileIndex = --preferredTiles[mi] * worker.size + mi;
        }

        if (tileIndex == -1)
          workerNotified[workerRankFromGlobalRank(requester)] = true;

        auto answer = std::make_shared<mpicommon::Message>(&tileIndex, sizeof(tileIndex));
        mpi::messaging::sendTo(requester, myId, answer);
      }

      float Master::renderFrame(Renderer *renderer
          , FrameBuffer *fb
          , const uint32 /*channelFlags*/
          )
      {
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        const int tilesTotal = fb->getTotalTiles();
        const int tilesPerWorker = tilesTotal / worker.size;
        const int remainingTiles = tilesTotal % worker.size;

        for (int w = 0; w < worker.size; w++) {
          preferredTiles[w] = tilesPerWorker + (w < remainingTiles);
          workerNotified[w] = false;
        }

        dfb->startNewFrame(renderer->errorThreshold);

        dfb->beginFrame();

        dfb->waitUntilFinished();

        return dfb->endFrame(renderer->errorThreshold);
      }

      std::string Master::toString() const
      {
        return "osp::mpi::dynamicLoadBalancer::Master";
      }


      // dynamicLoadBalancer::Slave definitions ////////////////////////////////

      Slave::Slave()
      {
        mpi::messaging::registerMessageListener(myId, this);

        auto OSPRAY_PREALLOCATED_TILES = getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");
        numPreAllocated = OSPRAY_PREALLOCATED_TILES.first ? OSPRAY_PREALLOCATED_TILES.second : 4;
        PRINT(numPreAllocated);
      }

      void Slave::incoming(const std::shared_ptr<mpicommon::Message> &msg)
      {
        auto tileIndex = *(ssize_t*)msg->data;

        mutex.lock();
        if (tileIndex == -1) {
          tilesAvailable = false;
        } else
          tilesScheduled++;
        mutex.unlock();

        if (tileIndex == -1)
          cv.notify_one();
        else
          tasking::schedule([&,tileIndex]{tileTask(tileIndex);});
      }

      float Slave::renderFrame(Renderer *_renderer
          , FrameBuffer *_fb
          , const uint32 channelFlags
          )
      {
        renderer = _renderer;
        fb = _fb;
        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        perFrameData = renderer->beginFrame(fb);

        tilesAvailable = true;
        tilesScheduled = numPreAllocated;
        for(int taskIndex = 0; taskIndex < numPreAllocated; taskIndex++) {
          tasking::schedule([&,taskIndex]{tileTask(taskIndex * worker.size + worker.rank);});
          // already "prefetch" a next tile
          requestTile();
        }

        dfb->waitUntilFinished();

        { // wait for render threads to finish
          std::unique_lock<std::mutex> lock(mutex);
          cv.wait(lock, [&]{ return tilesScheduled == 0 && !tilesAvailable; });
        }

        renderer->endFrame(perFrameData,channelFlags);

        return dfb->endFrame(inf); // irrelevant return value on slave, still
                                   // call to stop maml layer
      }

      void Slave::tileTask(const size_t tileID)
      {
        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID - tile_y*numTiles_x;
        const vec2i tileId(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileId);

        if (fb->tileError(tileId) <= renderer->errorThreshold)
          return;

#if TILE_SIZE > MAX_TILE_SIZE
        auto tilePtr = make_unique<Tile>(tileId, fb->size, accumID);
        auto &tile   = *tilePtr;
#else
        Tile __aligned(64) tile(tileId, fb->size, accumID);
#endif

        tasking::parallel_for(numJobs(renderer->spp, accumID), [&](int tid) {
          renderer->renderTile(perFrameData, tile, tid);
//          std::stringstream msg; msg << (int)((tileID - worker.rank)/worker.size) << " : " << tid << " \ttile " << tileID << std::endl; SCOPED_LOCK(mutex); std::cout << msg.str();
        });

        fb->setTile(tile);
//        if (worker.rank == 1) {std::stringstream msg; msg << taskIdx << " \ttile " << tileID << " DONE\n"; std::cout << msg.str();}

        if (tilesAvailable)
          requestTile();

        SCOPED_LOCK(mutex);
        if (--tilesScheduled == 0)
          cv.notify_one();
      }

      void Slave::requestTile()
      {
          int requester = mpi::globalRank();
          auto msg = std::make_shared<mpicommon::Message>(&requester, sizeof(requester));
          mpi::messaging::sendTo(mpi::masterRank(), myId, msg);
      }

      std::string Slave::toString() const
      {
        return "osp::mpi::dynamicLoadBalancer::Slave";
      }

    }// ::ospray::mpi::dynamicLoadBalancer
  } // ::ospray::mpi
} // ::ospray
