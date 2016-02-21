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

#pragma once

#include "ospray/mpi/MPICommon.h"
#include "ospray/render/LoadBalancer.h"

namespace ospray {
  namespace mpi {
    
    // =======================================================
    // =======================================================
    // =======================================================
    namespace staticLoadBalancer {
      /*! \brief the 'master' in a tile-based master-slave *static*
          load balancer

          The static load balancer assigns tiles based on a fixed pattern;
          right now simply based on a round-robin pattern (ie, each client
          'i' renderss tiles with 'tileID%numWorkers==i'
      */
      struct Master : public TiledLoadBalancer
      {
        void renderFrame(Renderer *tiledRenderer,
                         FrameBuffer *fb,
                         const uint32 channelFlags);
        std::string toString() const;
      };

      /*! \brief the 'slave' in a tile-based master-slave *static*
          load balancer

          The static load balancer assigns tiles based on a fixed pattern;
          right now simply based on a round-robin pattern (ie, each client
          'i' renderss tiles with 'tileID%numWorkers==i'
      */
      struct Slave : public TiledLoadBalancer
      {
        /*! a task for rendering a frame using the global tiled load balancer */
        //! NOTE(jda) - this looks identical to LocalTiledLoadBalancer...
        struct RenderTask
        {
          mutable Ref<Renderer>     renderer;
          mutable Ref<FrameBuffer>  fb;

          size_t                    numTiles_x;
          size_t                    numTiles_y;
          uint32                    channelFlags;
          void                     *perFrameData;

          void run(size_t jobID) const;
          void finish() const;
        };
        
        /*! number of tiles preallocated to this client; we can always
          render those even without asking for them. */
        uint32 numPreAllocated; 
        /*! total number of worker threads across all(!) slaves */
        int32 numTotalThreads;
        
        void renderFrame(Renderer *tiledRenderer,
                         FrameBuffer *fb,
                         const uint32 channelFlags);
        std::string toString() const;
      };
    }

  } // ::ospray::mpi
} // ::ospray
