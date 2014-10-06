/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/camera/perspectivecamera.h"
#include "ospray/volume/Volume.h"
#include "ospray/common/ray.h"
#include "ospray/common/Data.h"
#include "ospray/render/dvr/dvr.h"
#include "dvr_ispc.h"

namespace ospray {

  ISPCDVRRenderer::ISPCDVRRenderer()
  {
    ispcEquivalent = ispc::DVRRenderer_create(this);
  }

  void DVRRendererBase::commit()
  {
    Renderer::commit();

    /* for now, let's get the volume data from the renderer
       parameter. at some point we probably want to have voluems
       embedded in models - possibly with transformation attached to
       them - but for now let's do this as simple as possible */
    volume = (Volume *)this->getParamObject("volume",NULL);
    Assert(volume && "null volume handle in 'dvr' renderer "
           "(did you forget to assign a 'volume' parameter to the renderer?)");

    camera = (Camera *)getParamObject("camera",NULL);
    Assert(camera && "null camera handle in 'dvr' renderer "
           "(did you forget to assign a 'camera' parameter to the renderer?)");

    dt = getParam1f("dt", 0.0f);
  }


  void ISPCDVRRenderer::commit()
  {
    DVRRendererBase::commit();
    ispc::DVRRenderer_set(getIE(),camera->getIE(),volume->getIE(),dt);
  }

  OSP_REGISTER_RENDERER(ISPCDVRRenderer,dvr_ispc);
  OSP_REGISTER_RENDERER(ISPCDVRRenderer,dvr);

  extern "C" void ospray_init_module_dvr() 
  {
    printf("Loaded plugin 'dvr' ...\n");
  }
};

