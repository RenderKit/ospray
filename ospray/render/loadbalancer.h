#pragma once

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
  };

  struct LocalTiledLoadBalancer 
  {
    struct RenderTileTask : public embree::TaskScheduler::Task {
      RenderTileTask(size_t numTiles, 
                     LocalTiledLoadBalancer *loadBalancer,
                     TileRenderer *tiledRenderer,
                     FrameBuffer *fb);

      LocalTiledLoadBalancer  *loadBalancer;
      TaskScheduler::EventSync eventSync;
      TileRenderer            *tiledRenderer;
      FrameBuffer             *fb;
      // void run(size_t threadIndex, size_t threadCount, 
      //          size_t taskIndex, size_t taskCount, 
      //          embree::TaskScheduler::Event* event);
      TASK_RUN_FUNCTION(RenderTileTask,run);
      // TASK_COMPLETE_FUNCTION(ISPCTask,finish);
    };
  

    virtual void renderFrame(TileRenderer *tiledRenderer,
                             FrameBuffer *fb);
  };
}
