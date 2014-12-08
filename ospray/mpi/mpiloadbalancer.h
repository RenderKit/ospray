/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "mpicommon.h"
#include "../render/LoadBalancer.h"

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
        virtual std::string toString() const { return "ospray::mpi::staticLoadBalancer::Master"; };
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
          size_t                       numTiles_x;
          size_t                       numTiles_y;
          //          vec2i                        fbSize;
          uint32                       channelFlags;
          embree::TaskScheduler::Task  task;
          
          TASK_RUN_FUNCTION(RenderTask,run);
          TASK_COMPLETE_FUNCTION(RenderTask,finish);
          
          virtual ~RenderTask() {}
        };
        
        /*! number of tiles preallocated to this client; we can always
          render those even without asking for them. */
        uint32 numPreAllocated; 
        /*! total number of worker threads across all(!) slaves */
        int32 numTotalThreads;
        
        virtual void renderFrame(Renderer *tiledRenderer, 
                                 FrameBuffer *fb,
                                 const uint32 channelFlags);
        virtual std::string toString() const { return "ospray::mpi::staticLoadBalancer::Slave"; };
      };
    }

  }
}
