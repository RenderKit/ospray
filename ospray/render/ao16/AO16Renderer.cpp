// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "AO16Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/texture/Texture2D.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "SimpleAORenderer_ispc.h"

namespace ospray {

  //! Constructor
  template<int NUM_SAMPLES_PER_FRAME>
  SimpleAOMaterial<NUM_SAMPLES_PER_FRAME>::SimpleAOMaterial()
  {
    ispcEquivalent = ispc::SimpleAOMaterial_create(this);
  }
  
  /*! \brief common function to help printf-debugging */
  template<int NUM_SAMPLES_PER_FRAME>
  std::string SimpleAOMaterial<NUM_SAMPLES_PER_FRAME>::toString() const 
  {
    return "ospray::SimpleAORenderer"; 
  }
  
  /*! \brief create a material of given type */
  template<int NUM_SAMPLES_PER_FRAME>
  Material *SimpleAOMaterial<NUM_SAMPLES_PER_FRAME>::createMaterial(const char *type) 
  { 
    return new SimpleAOMaterial; 
  }

  void SimpleAOMaterial::commit() 
  {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
    map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd", NULL));
    ispc::SimpleAOMaterial_set(getIE(),
                           (const ispc::vec3f&)Kd,
                           map_Kd.ptr!=NULL?map_Kd->getIE():NULL);
  }
  
  template<int NUM_SAMPLES_PER_FRAME>
  SimpleAORenderer<NUM_SAMPLES_PER_FRAME>::SimpleAORenderer() 
    : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ispc::SimpleAORenderer_create(this,NULL,NULL);
  };

  template<int NUM_SAMPLES_PER_FRAME>
  void SimpleAORenderer<NUM_SAMPLES_PER_FRAME>::commit()
  {
    Renderer::commit();

    model  = (Model  *)getParamObject("world",NULL); // old naming
    model  = (Model  *)getParamObject("model",model); // new naming
    camera = (Camera *)getParamObject("camera",NULL);
    bgColor = getParam3f("bgColor",vec3f(1.f));
    ispc::SimpleAORenderer_set(getIE(),
                           NUM_SAMPLES_PER_FRAME,
                           (const ispc::vec3f&)bgColor,                           
                           model?model->getIE():NULL,
                           camera?camera->getIE():NULL);
  }

  typedef SimpleAORenderer<16> AO16Renderer;
  typedef SimpleAORenderer<8>  AO8Renderer;
  typedef SimpleAORenderer<4>  AO4Renderer;
  typedef SimpleAORenderer<2>  AO2Renderer;
  typedef SimpleAORenderer<1>  AO1Renderer;

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
  OSP_REGISTER_RENDERER(AO8Renderer, ao8);
  OSP_REGISTER_RENDERER(AO4Renderer, ao4);
  OSP_REGISTER_RENDERER(AO2Renderer, ao2);
  OSP_REGISTER_RENDERER(AO1Renderer, ao1);
  OSP_REGISTER_RENDERER(AO16Renderer,ao);

} // ::ospray

