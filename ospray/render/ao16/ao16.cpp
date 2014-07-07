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
    Kd = getParam3f("kd", getParam3f("Kd", vec3f(.8f)));
    map_Kd = NULL; //(Texture*)getParam("map_Kd",NULL);
    ispc::AO16Material_set(getIE(),(const ispc::vec3f&)Kd,
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
    ispc::AO16Renderer_set(getIE(),
                           model?model->getIE():NULL,
                           camera?camera->getIE():NULL);
  }

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
};

