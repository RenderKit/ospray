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

#define MAX_TILE_SIZE 128

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

#if TILE_SIZE > MAX_TILE_SIZE
          auto tilePtr = make_unique<Tile>(task.tileId, fb->size, task.accumId);
          auto &tile   = *tilePtr;
#else
          Tile __aligned(64) tile(tileID, dfb->size, accumID);
#endif

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
        scheduleTile(workerRankFromGlobalRank(requester));
      }

      void Master::scheduleTile(const int worker)
      {
        if (workerNotified[worker])
          return;

        // choose tile from preferred queue
        auto queue = preferredTiles.begin() + worker;

        // else look for largest non-empty queue
        if (queue->empty())
          queue = std::max_element(preferredTiles.begin(), preferredTiles.end(),
              [](TileVector a, TileVector b) { return a.size() < b.size(); });

        TileTask task;
        if (queue->empty()) {
          task.instances = 0; // no more tiles available
          workerNotified[worker] = true;
        } else {
          task = queue->back();
          queue->pop_back();
        }

        auto answer = std::make_shared<mpicommon::Message>(&task, sizeof(task));
        mpi::messaging::sendTo(globalRankFromWorkerRank(worker), myId, answer);
      }

      float Master::renderFrame(Renderer *renderer
          , FrameBuffer *fb
          , const uint32 /*channelFlags*/
          )
      {
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        // only duplicate tile together with region it is part of

        const int tilesTotal = fb->getTotalTiles();
        const int tilesPerWorker = tilesTotal / worker.size;
        const int remainingTiles = tilesTotal % worker.size;

        TileTask task;
        size_t tileId = 0;
        for (int y = 0; y < dfb->numTiles.y; y++) {
          for (int x = 0; x < dfb->numTiles.x; x++, tileId++) {
            task.tileId = vec2i(x, y);
            if (fb->tileError(task.tileId) <= renderer->errorThreshold)
              continue;

            // XXX accumID is incremented in DFB::startNewFrame, but we already
            // want to create the tasks here
            task.accumId = fb->accumID(task.tileId) + 1;
            task.instances = 1;

            auto nr = workerRankFromGlobalRank(dfb->ownerIDFromTileID(tileId));
            preferredTiles[nr].push_back(task);
          }
        }
        for(auto&& notified : workerNotified)
          notified = false;

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        for(int tiles = 0; tiles < numPreAllocated; tiles++)
          for(int workerID = 0; workerID < worker.size; workerID++)
            scheduleTile(workerID);

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
      }

      void Slave::incoming(const std::shared_ptr<mpicommon::Message> &msg)
      {
        auto task = *(TileTask*)msg->data;

        mutex.lock();
        if (task.instances == 0)
          tilesAvailable = false;
        else
          tilesScheduled++;
        mutex.unlock();

        if (tilesAvailable)
          tasking::schedule([&,task]{tileTask(task);});
        else
          cv.notify_one();
      }

      float Slave::renderFrame(Renderer *_renderer
          , FrameBuffer *_fb
          , const uint32 channelFlags
          )
      {
        renderer = _renderer;
        fb = _fb;
        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);

        tilesAvailable = true;
        tilesScheduled = 0;

        dfb->startNewFrame(renderer->errorThreshold);
        dfb->beginFrame();

        perFrameData = renderer->beginFrame(fb);
        frameActive = true;

        dfb->waitUntilFinished(); // swap with below?
        { // wait for render threads to finish
          std::unique_lock<std::mutex> lock(mutex);
          cv.wait(lock, [&]{ return tilesScheduled == 0 && !tilesAvailable; });
        }
        frameActive = false;

        renderer->endFrame(perFrameData,channelFlags);

        return dfb->endFrame(inf); // irrelevant return value on slave, still
                                   // call to stop maml layer
      }


      void Slave::tileTask(const TileTask &task)
      {
#if TILE_SIZE > MAX_TILE_SIZE
          auto tilePtr = make_unique<Tile>(task.tileId, fb->size, task.accumId);
          auto &tile   = *tilePtr;
#else
          Tile __aligned(64) tile(task.tileId, fb->size, task.accumId);
#endif

          while (!frameActive) PRINT(frameActive); // XXX busy wait for valid perFrameData

          tasking::parallel_for(numJobs(renderer->spp, task.accumId), [&](int tid) {
            renderer->renderTile(perFrameData, tile, tid);
          });

        if (tilesAvailable)
          requestTile(); // XXX here or after setTile?

        fb->setTile(tile);


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
