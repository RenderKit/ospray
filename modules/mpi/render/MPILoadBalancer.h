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

#pragma once

// ospray components
#include "components/mpiCommon/MPICommon.h"
#include "../common/Messaging.h"
// ours
#include "render/LoadBalancer.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include <condition_variable>

namespace ospray {
  namespace mpi {

    namespace staticLoadBalancer {
      /*! \brief the 'master' in a tile-based master-slave *static*
          load balancer

          The static load balancer assigns tiles based on a fixed pattern;
          right now simply based on a round-robin pattern (ie, each client
          'i' renderss tiles with 'tileID%numWorkers==i'
      */
      struct Master : public TiledLoadBalancer
      {
        float renderFrame(Renderer *tiledRenderer,
                          FrameBuffer *fb,
                          const uint32 channelFlags) override;
        std::string toString() const override;
      };

      /*! \brief the 'slave' in a tile-based master-slave *static*
          load balancer

          The static load balancer assigns tiles based on a fixed pattern;
          right now simply based on a round-robin pattern (ie, each client
          'i' renderss tiles with 'tileID%numWorkers==i'
      */
      struct Slave : public TiledLoadBalancer
      {
        float renderFrame(Renderer *tiledRenderer,
                          FrameBuffer *fb,
                          const uint32 channelFlags) override;
        std::string toString() const override;
      };

      struct Distributed : public TiledLoadBalancer
      {
        float renderFrame(Renderer *tiledRenderer,
                          FrameBuffer *fb,
                          const uint32 channelFlags) override;

        std::string toString() const override;
      };

    }// ::ospray::mpi::staticLoadBalancer

    namespace dynamicLoadBalancer {
      /*! \brief the 'master' in a tile-based master-slave load balancer

          The dynamic load balancer assigns tiles asynchronously, favouring the
          same tiles as the DistributedFramebuffer (i.e. round-robin pattern,
          each client 'i' renderss tiles with 'tileID%numWorkers==i') to avoid
          transferring a computed tile for accumulation
      */

      struct TileTask {
        vec2i tileId;
        int32 accumId;
        bool tilesExhausted; // no more tiles available
      };

      class Master : public messaging::MessageHandler,
                     public TiledLoadBalancer
      {
      public:
        Master(ObjectHandle handle, int numPreAllocated = 4);
        void incoming(const std::shared_ptr<mpicommon::Message> &) override;
        float renderFrame(Renderer *tiledRenderer
            , FrameBuffer *fb
            , const uint32 channelFlags
            ) override;
        std::string toString() const override;

      private:
        void scheduleTile(const int worker);
        void generateTileTasks(DistributedFrameBuffer * const dfb
            , const float errorThreshold
            );

        typedef std::vector<TileTask> TileVector;
        std::vector<TileVector> preferredTiles; // per worker default queue
        std::vector<bool> workerNotified; // worker knows we're done?
        int numPreAllocated{4};
      };

      /*! \brief the 'slave' in a tile-based master-slave load balancer

      */
      class Slave : public messaging::MessageHandler,
                    public TiledLoadBalancer
      {
      public:
        Slave(ObjectHandle handle);
        void incoming(const std::shared_ptr<mpicommon::Message> &) override;
        float renderFrame(Renderer *tiledRenderer
            , FrameBuffer *fb
            , const uint32 channelFlags
            ) override;
        std::string toString() const override;

      private:
        void tileTask(const TileTask &task);
        void requestTile();

        // "local" state
        Renderer *renderer;
        FrameBuffer *fb;
        void *perFrameData;

        std::mutex mutex;
        std::condition_variable cv;
        int tilesScheduled;
        bool tilesAvailable;
        bool frameActive;
      };

    }// ::ospray::mpi::dynamicLoadBalancer
  } // ::ospray::mpi
} // ::ospray
