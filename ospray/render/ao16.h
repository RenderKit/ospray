#pragma once

// ospray
#include "tilerenderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  /*! test renderer that renders a simple test image using ispc */
  struct AO16Renderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::AO16Renderer"; }

    struct RenderTask : public TileRenderer::RenderJob {
      Model  *world;
      Camera *camera;
      virtual void renderTile(Tile &tile);
      virtual ~RenderTask() {}
    };
    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
  };
};
