#pragma once

#include "render/tilerenderer.h"

namespace ospray {
  struct Camera;
  struct Model;

  namespace rv {

    /*! test renderer that renders a simple test image using ispc */
    struct RVRenderer : public TileRenderer {
      virtual std::string toString() const { return "ospray::rv::RVRenderer"; }
      virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

      struct TileJob : public TileRenderer::RenderJob {
        /*! \brief render given tile */
        virtual void renderTile(Tile &tile);

        void *ispc_camera;
        void *ispc_scene;
      };
    };
  }

};
