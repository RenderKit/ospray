/*! \defgroup ospray_module_dvr Experimental Direct Volume Rendering (DVR) Module

  \ingroup ospray_modules

  \brief Experimental direct volume rendering (DVR) functionality 

  Implements both scalar and ISPC-vectorized DVR renderers for various
  volume data layouts. Much of that functionality will eventually flow
  back into the ospray core (some already has); this module is mostly
  for experimental code (e.g., different variants of data structures
  or sample codes) that we do not (yet) want to have in the main
  ospray codebase (being a module allows this code to be conditionally
  enabled/disabled, and allows ospray to be built even if the dvr
  module for some reason doesn't build).

*/

#pragma once

#include "render/tilerenderer.h"

/*! @{ \ingroup ospray_module_dvr */
namespace ospray {
  struct Camera;
  struct Model;
  struct Volume;

  /*! test renderer that renders a simple test image using ispc */
  struct ISPCDVRRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

    struct TileJob : public TileRenderer::RenderJob {
      /*! \brief render given tile */
      virtual void renderTile(Tile &tile);

      void *ispc_volume;
      void *ispc_camera;
      float dt;
    };
  };

  /*! test renderer that renders a simple test image using ispc */
  struct ScalarDVRRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

    struct TileJob : public TileRenderer::RenderJob {
      /*! \brief render given tile */
      virtual void renderTile(Tile &tile);
      
      Camera *camera;
      Volume *volume;
      float dt;
    };
  };
}
/*! @} */

