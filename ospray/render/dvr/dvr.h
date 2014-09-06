/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



#pragma once

#include "ospray/render/renderer.h"

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
  struct DVRRendererBase : public Renderer {
    virtual void commit();

    Ref<TransferFct> transferFct;
    Ref<Camera>      camera;
    Ref<Volume>      volume;
    float dt;

    // virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);

    // struct RenderJob : public TileRenderer::RenderJob {
    //   /*! \brief render given tile */
    //   virtual void renderTile(Tile &tile);
      
    //   Ref<Camera>      camera;
    //   Ref<Volume>      volume;
    //   Ref<TransferFct> transferFct;
    //   Ref<DVRRendererBase> renderer;
    //   float dt;
    // };
    // virtual void renderTileDVR(Tile &tile, RenderJob *job) = 0;
  };

  /*! test renderer that renders a simple test image using ispc */
  struct ISPCDVRRenderer : public DVRRendererBase {
    ISPCDVRRenderer();
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual void commit();
    // virtual void renderTileDVR(Tile &tile, DVRRendererBase::RenderJob *job);

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

  // /*! test renderer that renders a simple test image using ispc */
  // struct ScalarDVRRenderer : public DVRRendererBase {
  //   virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
  //   virtual void renderTileDVR(Tile &tile, DVRRendererBase::RenderJob *job);

  //   // virtual TileRenderer::RenderJob *createRenderJob(FrameBuffer *fb);
  //   // Ref<TransferFct> transferFct;
  // };
}
/*! @} */

