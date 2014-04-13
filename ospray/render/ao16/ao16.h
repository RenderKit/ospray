#pragma once

/*! \group ospray_render_ao16 Simple 16-sample Ambient Occlusion Renderer
  
  \ingroup ospray_renderer

  Implements a simple renderer that shoots 16 rays (generated using a
  hard-coded set of random numbers) to compute a trivially simple and
  coarse ambient occlusion effect 

*/

// ospray
#include "tilerenderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  /*! Simple 16-sample Ambient Occlusion Renderer */
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
