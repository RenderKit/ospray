/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ospray/render/renderer.h"

namespace ospray {
  /*! \brief Implements a simple "Direct Volume (ray casting) Renderer" (DVR) */
  struct DVRRenderer : public Renderer {
    virtual std::string toString() const { return "ospray::DVRRenderer"; }
    virtual void renderFrame(FrameBuffer *fb);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
