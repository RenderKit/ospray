#pragma once

/*! \file loadbalancer.h Implements the abstracion layer for a (tiled) load balancer */

// ospray
#include "../common/ospcommon.h"
#include "../fb/framebuffer.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  struct TileRenderer;

  struct TiledLoadBalancer 
  {
    static TiledLoadBalancer *instance;
    virtual void renderFrame(TileRenderer *tiledRenderer,
                             FrameBuffer *fb) = 0;
    virtual void returnTile(FrameBuffer *fb, Tile &tile) = 0;
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
      rendering on a local machine, without any cross-node
      communication/load balancing at all (even if there are multiple
      application ranks each doing local rendering on their own)  */ 
  struct LocalTiledLoadBalancer : public TiledLoadBalancer
  {
    /*! a task for rendering a frame using the global tiled load balancer */
    struct RenderTileTask : public embree::TaskScheduler::Task {
      RenderTileTask(size_t numTiles, 
                     TileRenderer *tiledRenderer,
                     FrameBuffer *fb);
      TileRenderer            *tiledRenderer;
      FrameBuffer             *fb;
      TASK_RUN_FUNCTION(RenderTileTask,run);
      TASK_COMPLETE_FUNCTION(RenderTileTask,finish);
    };

    virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
    virtual void returnTile(FrameBuffer *fb, Tile &tile);
  };
}
