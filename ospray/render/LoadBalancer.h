#pragma once

/*! \file LoadBalancer.h Implements the abstracion layer for a (tiled) load balancer */

// ospray
#include "ospray/common/OSPCommon.h"
#include "ospray/fb/FrameBuffer.h"
#include "ospray/render/Renderer.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {

  using embree::TaskScheduler;

  struct TileRenderer;

  struct TiledLoadBalancer 
  {
    static TiledLoadBalancer *instance;
    virtual std::string toString() const = 0;
    virtual void renderFrame(Renderer *tiledRenderer,
                             FrameBuffer *fb,
                             const uint32 channelFlags) = 0;
    // virtual void returnTile(FrameBuffer *fb, Tile &tile) = 0;
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
    rendering on a local machine, without any cross-node
    communication/load balancing at all (even if there are multiple
    application ranks each doing local rendering on their own)  */ 
  struct LocalTiledLoadBalancer : public TiledLoadBalancer
  {
    struct RenderTask : public embree::RefCount {
      Ref<FrameBuffer>             fb;
      Ref<Renderer>                renderer;
      
      size_t                       numTiles_x;
      size_t                       numTiles_y;
      uint32                       channelFlags;
      embree::TaskScheduler::Task  task;

      TASK_RUN_FUNCTION(RenderTask,run);
      TASK_COMPLETE_FUNCTION(RenderTask,finish);
    };

    virtual void renderFrame(Renderer *tiledRenderer, 
                             FrameBuffer *fb,
                             const uint32 channelFlags);
    virtual std::string toString() const { return "ospray::LocalTiledLoadBalancer"; };
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
    rendering on a local machine, without any cross-node
    communication/load balancing at all (even if there are multiple
    application ranks each doing local rendering on their own)  */ 
  struct InterleavedTiledLoadBalancer : public TiledLoadBalancer
  {
    size_t deviceID;
    size_t numDevices;

    InterleavedTiledLoadBalancer(size_t deviceID, size_t numDevices)
      : deviceID(deviceID), numDevices(numDevices)
    {
      if (ospray::debugMode || ospray::logLevel) {
        std::cout << "=======================================================" << std::endl;
        std::cout << "INTERLEAVED LOAD BALANCER" << std::endl;
        std::cout << "=======================================================" << std::endl;
      }
    }

    virtual std::string toString() const { return "ospray::InterleavedTiledLoadBalancer"; };

    /*! \brief a task for rendering a frame using the global tiled load balancer 
      
      if derived from FrameBuffer::RenderFrameEvent to allow us for
      attaching that as a sync primitive to the farme buffer
    */
    struct RenderTask : public embree::RefCount {
      Ref<FrameBuffer>             fb;
      Ref<Renderer>                renderer;
      
      size_t                       numTiles_x;
      size_t                       numTiles_y;
      size_t                       numTiles_mine;
      size_t                       deviceID;
      size_t                       numDevices;
      uint32                       channelFlags;
      embree::TaskScheduler::Task  task;

      TASK_RUN_FUNCTION(RenderTask,run);
      TASK_COMPLETE_FUNCTION(RenderTask,finish);
    };
    
    virtual void renderFrame(Renderer *tiledRenderer, 
                             FrameBuffer *fb,
                             const uint32 channelFlags);
  };

} // ::ospray
