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

  /*! \brief a transfer fct for classifying (float) scalar values to RGBA color values

    at some point in time we'll probably want a higher-level
    abstraction for that; for now we'll only support a uniform
    transfer fct texture */
  struct TransferFct : public ManagedObject {
    Ref<Data> data; //!< the data array containing the xfer fct (if obtained from data)
    vec4f *colorPerBin;
    int    numBins;

    TransferFct(Data *data);
    TransferFct();
  };

  /*! abstract DVR renderer that includes shared functionality for
      both ispc and scalar rendering */
  struct DVRRendererBase : public TileRenderer {
    virtual void commit();

    Ref<TransferFct> transferFct;
    Ref<Camera>      camera;
    Ref<Volume>      volume;

    virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

    struct RenderJob : public TileRenderer::RenderJob {
      /*! \brief render given tile */
      virtual void renderTile(Tile &tile);
      
      Ref<Camera>      camera;
      Ref<Volume>      volume;
      Ref<TransferFct> transferFct;
      Ref<DVRRendererBase> renderer;
      float dt;
    };
    virtual void renderTileDVR(Tile &tile, RenderJob *job) = 0;
  };

  /*! test renderer that renders a simple test image using ispc */
  struct ISPCDVRRenderer : public DVRRendererBase {
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual void renderTileDVR(Tile &tile, DVRRendererBase::RenderJob *job);

    // virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
    // struct TileJob : public TileRenderer::RenderJob {
    //   /*! \brief render given tile */
    //   virtual void renderTile(Tile &tile);

    //   void *ispc_volume;
    //   void *ispc_camera;
    //   Ref<TransferFct> transferFct;
    //   float dt;
    // };
  };

  /*! test renderer that renders a simple test image using ispc */
  struct ScalarDVRRenderer : public DVRRendererBase {
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual void renderTileDVR(Tile &tile, DVRRendererBase::RenderJob *job);

    // virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
    // Ref<TransferFct> transferFct;
  };
}
/*! @} */

