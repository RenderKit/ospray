// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "ospcommon/utility/getEnvVar.h"
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
        // dfb->beginFrame(); is called by renderer->beginFrame:
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

          if (dfb->continueRendering()) {
            tasking::parallel_for(numJobs(renderer->spp, accumID),
                                  [&](size_t tid) {
              renderer->renderTile(perFrameData, tile, tid);
            });
          }

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

      float Distributed::renderFrame(Renderer *, FrameBuffer *, const uint32)
      {
        throw std::runtime_error("Distributed renderers must implement their"
                                 " own renderFrame()! Distributed rendering"
                                 " algorithms are highly coupled to how"
                                 " communication between nodes operate. Thus"
                                 " there isn't a 'default' implementation as"
                                 " there is with the ISPCDevice or "
                                 " MPIOffloadDevice.");
      }

      std::string Distributed::toString() const
      {
        return "ospray::mpi::staticLoadBalancer::Distributed[placeholder]";
      }

    }// ::ospray::mpi::staticLoadBalancer

    namespace dynamicLoadBalancer {

      // dynamicLoadBalancer::Master definitions ///////////////////////////////

      Master::Master(ObjectHandle handle, int _numPreAllocated)
        : MessageHandler(handle), numPreAllocated(_numPreAllocated)
      {
        preferredTiles.resize(worker.size);
        workerNotified.resize(worker.size);

        // TODO numPreAllocated should be estimated/tuned automatically
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
          workerNotified[worker] = true;
          task.tilesExhausted = true;
        } else {
          task = queue->back();
          queue->pop_back();
          task.tilesExhausted = false;
        }

        auto answer = std::make_shared<mpicommon::Message>(&task, sizeof(task));
        mpi::messaging::sendTo(globalRankFromWorkerRank(worker), myId, answer);
      }

      void Master::generateTileTasks(DistributedFrameBuffer * const dfb
          , const float errorThreshold
          )
      {
        struct ActiveTile {
          float error;
          vec2i id;
        };
        std::vector<ActiveTile> activeTiles;
        TileTask task;
        for (int y = 0, tileNr = 0; y < dfb->numTiles.y; y++) {
          for (int x = 0; x < dfb->numTiles.x; x++, tileNr++) {
            const auto tileId = vec2i(x, y);
            const auto tileError = dfb->tileError(tileId);
            if (tileError <= errorThreshold)
              continue;

            // remember active tiles
            activeTiles.push_back({tileError, tileId});

            task.tileId = tileId;
            task.accumId = dfb->accumID(tileId);

            auto nr = workerRankFromGlobalRank(dfb->ownerIDFromTileID(tileNr));
            preferredTiles[nr].push_back(task);
          }
        }

        if (activeTiles.empty())
          return;

        // sort active tiles, highest error first
        std::sort(activeTiles.begin(), activeTiles.end(),
            [](ActiveTile a, ActiveTile b) { return a.error > b.error; });

        // TODO: estimate variance reduction to avoid duplicating tiles that are
        // just slightly above errorThreshold too often
        auto it = activeTiles.begin();
        const size_t tilesTotal = dfb->getTotalTiles();
        // loop over (active) tiles multiple times (instead of e.g. computing
        // instance count) to have maximum distance between duplicated tiles in
        // queue ==> higher chance that duplicated tiles do not arrive at the
        // same time at DFB and thus avoid the mutex in WriteMultipleTile::process
        for (auto i = activeTiles.size(); i < tilesTotal; i++) {
          const auto tileId = it->id;
          task.tileId = tileId;
          task.accumId = dfb->accumID(tileId);
          const auto tileNr = tileId.y*dfb->numTiles.x + tileId.x;
          auto nr = workerRankFromGlobalRank(dfb->ownerIDFromTileID(tileNr));
          preferredTiles[nr].push_back(task);

          if (++it == activeTiles.end())
            it = activeTiles.begin(); // start again from beginning
        }
      }

      float Master::renderFrame(Renderer *renderer
          , FrameBuffer *fb
          , const uint32 /*channelFlags*/
          )
      {
        DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        assert(dfb);

        for (size_t i = 0; i < workerNotified.size(); ++i)
          workerNotified[i] = false;

        generateTileTasks(dfb, renderer->errorThreshold);

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

      Slave::Slave(ObjectHandle handle) : MessageHandler(handle)
      {
      }

      void Slave::incoming(const std::shared_ptr<mpicommon::Message> &msg)
      {
        auto task = *(TileTask*)msg->data;

        mutex.lock();
        if (task.tilesExhausted)
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
        // dfb->beginFrame(); is called by renderer->beginFrame:
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

        while (!frameActive);// PRINT(frameActive); // XXX busy wait for valid perFrameData

        auto *dfb = dynamic_cast<DistributedFrameBuffer*>(fb);
        if (dfb->continueRendering()) {
          tasking::parallel_for(numJobs(renderer->spp, task.accumId),
                                [&](size_t tid) {
            renderer->renderTile(perFrameData, tile, tid);
          });
        }

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
        auto msg =
            std::make_shared<mpicommon::Message>(&requester, sizeof(requester));
        mpi::messaging::sendTo(mpi::masterRank(), myId, msg);
      }

      std::string Slave::toString() const
      {
        return "osp::mpi::dynamicLoadBalancer::Slave";
      }

    }// ::ospray::mpi::dynamicLoadBalancer
  } // ::ospray::mpi
} // ::ospray
