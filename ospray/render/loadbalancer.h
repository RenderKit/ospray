#pragma once

/*! \file loadbalancer.h Implements the abstracion layer for a (tiled) load balancer */

// ospray
#include "../common/ospcommon.h"
#include "../fb/framebuffer.h"
// embree
#include "common/sys/taskscheduler.h"
#include "render/tilerenderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct TileRenderer;

  struct TiledLoadBalancer 
  {
    static TiledLoadBalancer *instance;
    virtual void renderFrame(TileRenderer *tiledRenderer,
                             FrameBuffer *fb) = 0;
    // virtual void returnTile(FrameBuffer *fb, Tile &tile) = 0;
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
      rendering on a local machine, without any cross-node
      communication/load balancing at all (even if there are multiple
      application ranks each doing local rendering on their own)  */ 
  struct LocalTiledLoadBalancer : public TiledLoadBalancer
  {
    /*! \brief a task for rendering a frame using the global tiled load balancer 
      
      if derived from FrameBuffer::RenderFrameEvent to allow us for
      attaching that as a sync primitive to the farme buffer
     */
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

    virtual void renderFrame(TileRenderer *tiledRenderer, FrameBuffer *fb);
    // virtual void returnTile(FrameBuffer *fb, Tile &tile);
  };
}
