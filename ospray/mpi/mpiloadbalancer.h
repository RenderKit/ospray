#pragma once

#include "mpicommon.h"
#include "../render/loadbalancer.h"

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
        Master();
        
        virtual void renderFrame(Renderer *tiledRenderer,
                                 FrameBuffer *fb,
                                 const uint32 channelFlags);
      };

      /*! \brief the 'slave' in a tile-based master-slave *static*
          load balancer

          The static load balancer assigns tiles based on a fixed pattern;
          right now simply based on a round-robin pattern (ie, each client
          'i' renderss tiles with 'tileID%numWorkers==i'
      */
      struct Slave : public TiledLoadBalancer
      {
        Slave();
        
        /*! a task for rendering a frame using the global tiled load balancer */
        struct RenderTask : public embree::RefCount {
          Ref<Renderer>                renderer;
          Ref<FrameBuffer>             fb;
          embree::TaskScheduler::Task  task;
          size_t                       numTiles_x;
          size_t                       numTiles_y;
          //          vec2i                        fbSize;
          uint32                       channelFlags;
          
          TASK_RUN_FUNCTION(RenderTask,run);
          TASK_COMPLETE_FUNCTION(RenderTask,finish);
          
          virtual ~RenderTask() { }
        };
        
        /*! number of tiles preallocated to this client; we can always
          render those even without asking for them. */
        uint32 numPreAllocated; 
        /*! total number of worker threads across all(!) slaves */
        int32 numTotalThreads;
        
        virtual void renderFrame(Renderer *tiledRenderer, 
                                 FrameBuffer *fb,
                                 const uint32 channelFlags);
      };
    }

  }
}
