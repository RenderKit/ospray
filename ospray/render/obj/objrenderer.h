#pragma once


// ospray
#include "ospray/render/tilerenderer.h"
#include "ospray/common/material.h"
// obj renderer

namespace ospray {
  struct Camera;
  struct Model;

  namespace obj {
    using embree::TaskScheduler;

    /*! \brief Renderer for the OBJ Wavefront Material/Lighting format 

      See \ref ospray_render_obj
    */
    struct OBJRenderer : public TileRenderer {
      virtual std::string toString() const { return "ospray::OBJRenderer"; }

      struct RenderTask : public TileRenderer::RenderJob {
        Model   *world;
        Camera  *camera;
        Data    *materialData;
        Data    *textureData;
        virtual void renderTile(Tile &tile);
        virtual ~RenderTask() {}
      };
      virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
      /*! \brief create a material of given type */
      virtual Material *createMaterial(const char *type);
    };

  }
}

