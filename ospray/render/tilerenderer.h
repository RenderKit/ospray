/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \file tilerenderer.h Defines the base class for any tile-based renderer */

#include "renderer.h"

DEPRECATED - TILE RNDERER MERGED WITH RENDERER

// namespace ospray {
  
//   /*! \brief abstract for all renderers that use tiled load balancing 

//     it's up to the load balancer to call this renderer on a given set
//     of tiles; there is no guarantee which tiles this tile renderer's
//     'renderTile' function is being called, nor even how many such
//     calls are made for any given frame (for dynamic load balancing the
//     set of tiles may change from frame to frame
//    */
//   struct TileRenderer : public Renderer {
//     virtual void beginFrame(FrameBuffer *fb);
//     virtual void endFrame(FrameBuffer *fb);
//     // virtual void renderTile(FrameBuffer *fb, Tile &tile);
//     //! \brief common function to help printf-debugging 
//     virtual std::string toString() const { return "ospray::TileRenderer"; }

//     virtual void renderFrame(FrameBuffer *fb);
//   };

// }


