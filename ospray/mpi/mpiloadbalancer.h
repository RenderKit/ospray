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
        
        virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
        // virtual void returnTile(FrameBuffer *fb, Tile &tile);
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
        struct RenderTask : public FrameBuffer::RenderFrameEvent {
          RenderTask(FrameBuffer *fb,
                     TileRenderer::RenderJob *frameRenderJob);
          Ref<TileRenderer::RenderJob> frameRenderJob;
          Ref<FrameBuffer>             fb;
          TaskScheduler::Task          task;
          size_t                       numTiles_x;
          size_t                       numTiles_y;
          vec2i                        fbSize;
          
          TASK_RUN_FUNCTION(RenderTask,run);
          TASK_COMPLETE_FUNCTION(RenderTask,finish);
          
          virtual ~RenderTask() { }
        };
        
        /*! number of tiles preallocated to this client; we can always
          render those even without asking for them. */
        uint32 numPreAllocated; 
        /*! total number of worker threads across all(!) slaves */
        int32 numTotalThreads;
        
        virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
        // virtual void returnTile(FrameBuffer *fb, Tile &tile);
      };
    }

#if 0
    // =======================================================
    // =======================================================
    // =======================================================

    /*! \brief the 'master' in a tile-based master-slave dynamic load
        balancing in config.

        Right now this is implemented in a single thread; eventually
        we'll have to create a multi-threaded implementation in which
        each slave has its own receiver thread
    */
    struct DynamicLoadBalancer_Master : public TiledLoadBalancer
    {
      DynamicLoadBalancer_Master();
      
      /*! a task for rendering a frame using the global tiled load balancer */
      // struct MasterTask : public embree::TaskScheduler::Task {
      //   MasterTask(size_t numTiles, 
      //              TileRenderer *tiledRenderer,
      //              FrameBuffer *fb);
      //   TileRenderer            *tiledRenderer;
      //   FrameBuffer             *fb;
      //   TASK_RUN_FUNCTION(MasterTask,run);
      // };

      struct SlaveInfo {
        //! number of threads on this slave
        int32 numThreads;
        //! num tiles preallocated to this client (each frame)
        int32 numPreAllocated;
        //! num tiles returned this frame
        int32 numReturned;
      };
      SlaveInfo *slaveInfo;
      int32 numSlaveThreads;
      int32 numPreAllocated;
      
      virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
      // virtual void returnTile(FrameBuffer *fb, Tile &tile);
    };

    /*! \brief one of the 'slaves' in a tile-based master/slave
        dynamic load balancing config.

        Right now this is implemented in a single thread; eventually
        we'll have to create a multi-threaded implementation in which
        each slave has its own receiver thread
    */
    struct DynamicLoadBalancer_Slave : public TiledLoadBalancer
    {
      DynamicLoadBalancer_Slave();
      
      /*! a task for rendering a frame using the global tiled load balancer */
      struct SlaveTask : public embree::TaskScheduler::Task {
        SlaveTask(size_t numTiles, 
                  TileRenderer *tiledRenderer,
                  FrameBuffer *fb);
        TileRenderer            *tiledRenderer;
        FrameBuffer             *fb;
        TaskScheduler::EventSync doneRendering;

        TASK_RUN_FUNCTION(SlaveTask,run);
      };

      /*! number of tiles preallocated to this client; we can always
        render those even without asking for them. */
      uint32 numPreAllocated; 
      /*! total number of worker threads across all(!) slaves */
      int32 numTotalThreads;

      virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
      // virtual void returnTile(FrameBuffer *fb, Tile &tile);
    };
#endif
  }
}
