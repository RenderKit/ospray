/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray
#include "ao16.h"
#include "ospray/camera/camera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "ao16_ispc.h"

namespace ospray {

  AO16Material::AO16Material()
  {
    ispcEquivalent = ispc::AO16Material_create(this);
  }
  
  void AO16Material::commit() {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
    map_Kd = NULL; //(Texture*)getParam("map_Kd",NULL);
    ispc::AO16Material_set(getIE(),
                           (const ispc::vec3f&)Kd,
                           map_Kd!=NULL?map_Kd->getIE():NULL);
  }
  
  AO16Renderer::AO16Renderer() 
  : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ispc::AO16Renderer_create(this,NULL,NULL);
  };

  void AO16Renderer::commit()
  {
    Renderer::commit();

    model  = (Model  *)getParamObject("world",NULL); // old naming
    model  = (Model  *)getParamObject("model",model); // new naming
    camera = (Camera *)getParamObject("camera",NULL);
    bgColor = getParam3f("bgColor",vec3f(1.f));
    ispc::AO16Renderer_set(getIE(),
                           (const ispc::vec3f&)bgColor,                           
                           model?model->getIE():NULL,
                           camera?camera->getIE():NULL);
  }

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
  OSP_REGISTER_RENDERER(AO16Renderer,ao);
};

