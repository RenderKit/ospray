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

  /*! abstract DVR renderer that includes shared functionality for
      both ispc and scalar rendering */
  struct DVRRendererBase : public Renderer {
    virtual void commit();

    Ref<Camera>      camera;
    Ref<Volume>      volume;
    float dt;
  };

  /*! test renderer that renders a simple test image using ispc */
  struct ISPCDVRRenderer : public DVRRendererBase {
    ISPCDVRRenderer();
    virtual std::string toString() const { return "ospray::ISPCDVRRenderer"; }
    virtual void commit();
  };

}
/*! @} */

