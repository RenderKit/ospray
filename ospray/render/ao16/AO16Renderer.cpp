// ospray
#include "AO16Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/texture/Texture2D.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "AO16Renderer_ispc.h"

namespace ospray {

  AO16Material::AO16Material()
  {
    ispcEquivalent = ispc::AO16Material_create(this);
  }
  
  void AO16Material::commit() {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
    map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd", NULL));
    ispc::AO16Material_set(getIE(),
                           (const ispc::vec3f&)Kd,
                           map_Kd.ptr!=NULL?map_Kd->getIE():NULL);
  }
  
  template<int NUM_SAMPLES_PER_FRAME>
  AO16Renderer<NUM_SAMPLES_PER_FRAME>::AO16Renderer() 
  : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ispc::AO16Renderer_create(this,NULL,NULL);
  };

  template<int NUM_SAMPLES_PER_FRAME>
  void AO16Renderer<NUM_SAMPLES_PER_FRAME>::commit()
  {
    Renderer::commit();

    model  = (Model  *)getParamObject("world",NULL); // old naming
    model  = (Model  *)getParamObject("model",model); // new naming
    camera = (Camera *)getParamObject("camera",NULL);
    bgColor = getParam3f("bgColor",vec3f(1.f));
    ispc::AO16Renderer_set(getIE(),
                           NUM_SAMPLES_PER_FRAME,
                           (const ispc::vec3f&)bgColor,                           
                           model?model->getIE():NULL,
                           camera?camera->getIE():NULL);
  }

  typedef AO16Renderer<16> _AO16Renderer;
  typedef AO16Renderer<8>  _AO8Renderer;
  typedef AO16Renderer<4>  _AO4Renderer;
  typedef AO16Renderer<2>  _AO2Renderer;
  typedef AO16Renderer<1>  _AO1Renderer;

  OSP_REGISTER_RENDERER(_AO16Renderer,ao16);
  OSP_REGISTER_RENDERER(_AO8Renderer, ao8);
  OSP_REGISTER_RENDERER(_AO4Renderer, ao4);
  OSP_REGISTER_RENDERER(_AO2Renderer, ao2);
  OSP_REGISTER_RENDERER(_AO1Renderer, ao1);
  OSP_REGISTER_RENDERER(_AO16Renderer,ao);

} // ::ospray

