#pragma once

// ospray
#include "renderer.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  /*! test renderer that renders a simple test image using ispc */
  struct RayCastRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::RayCastRenderer"; }
    //virtual void renderFrame(FrameBuffer *fb);
    virtual void renderTile(Tile &tile);

    // struct RenderTask : public embree::TaskScheduler::Task {
    //   // RenderTask() : Task(NULL,_run,this,2,NULL,this,"raycast::rendertask") {}
    //   RenderTask() : Task(&sync,_run,NULL,2,NULL,NULL,"raycast::rendertask") {}

    //   TASK_RUN_FUNCTION(RayCastRenderer::RenderTask,run); //task_renderTiles);
    //   TASK_COMPLETE_FUNCTION(RayCastRenderer::RenderTask,finish);

    //   TaskScheduler::EventSync sync;
    //   virtual ~RenderTask() { PING; }
    // };
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
