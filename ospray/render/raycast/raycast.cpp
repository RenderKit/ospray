/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// ospray
#include "raycast.h"
#include "ospray/camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "raycast_ispc.h"

namespace ospray {

  template<void *(*ISPC_CREATE)(void*)>
  RayCastRenderer<ISPC_CREATE>::RayCastRenderer()
    : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ISPC_CREATE(this);
  }


  template<void *(*ISPC_CREATE)(void*)>
  void RayCastRenderer<ISPC_CREATE>::commit()
  {
    Renderer::commit();

    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    camera = (Camera *)getParamObject("camera",NULL);

    ispc::RayCastRenderer_set(getIE(),
                              model?model->getIE():NULL,
                              camera?camera->getIE():NULL);
  }

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight> RayCastRenderer_eyeLight;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,raycast);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,eyeLight);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,raycast_eyelight);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_Ng> RayCastRenderer_Ng;
  OSP_REGISTER_RENDERER(RayCastRenderer_Ng,raycast_Ng);
  typedef RayCastRenderer<ispc::RayCastRenderer_create_Ns> RayCastRenderer_Ns;
  OSP_REGISTER_RENDERER(RayCastRenderer_Ns,raycast_Ns);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_primID>
  RayCastRenderer_eyeLight_primID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_primID,eyeLight_primID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_primID,primID);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_instID>
  RayCastRenderer_eyeLight_instID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_instID,eyeLight_instID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_instID,instID);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_geomID>
  RayCastRenderer_eyeLight_geomID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_geomID,eyeLight_geomID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_geomID,geomID);

};

