#pragma once

// ospray
#include "tilerenderer.h"

namespace ospray {
  using embree::TaskScheduler;

  struct Camera;
  struct Model;

  enum { 
    RC_EYELIGHT=0,
    RC_PRIMID,
    RC_GEOMID,
    RC_INSTID,
    RC_GNORMAL
  } RC_SHADEMODE;

  /*! test renderer that renders a simple test image using ispc */
  template<int SHADE_MODE=RC_EYELIGHT>
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
