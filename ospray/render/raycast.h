#pragma once

// ospray
#include "renderer.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  /*! test renderer that renders a simple test image using ispc */
  struct RayCastRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::RayCastRenderer"; }

    struct RenderTask : public TileRenderer::RenderJob {
      Model  *world;
      void   *_scene;
      Camera *camera;
      void   *_camera;
      virtual void renderTile(Tile &tile);
      virtual ~RenderTask() {}
    };
    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
