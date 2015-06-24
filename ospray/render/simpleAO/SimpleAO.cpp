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
#include "SimpleAO.h"
#include "ospray/camera/Camera.h"
#include "ospray/texture/Texture2D.h"
// ispc exports
#include "SimpleAO_ispc.h"

namespace ospray {

  //! Constructor
  template<int NUM_SAMPLES_PER_FRAME>
  SimpleAO<NUM_SAMPLES_PER_FRAME>::Material::Material()
  {
    ispcEquivalent = ispc::SimpleAOMaterial_create(this);
  }
  
  template<int NUM_SAMPLES_PER_FRAME>
  void SimpleAO<NUM_SAMPLES_PER_FRAME>::Material::commit() 
  {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
    map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd", NULL));
    ispc::SimpleAOMaterial_set(getIE(),
                           (const ispc::vec3f&)Kd,
                           map_Kd.ptr!=NULL?map_Kd->getIE():NULL);
  }
  
  //! \brief Constructor
  template<int NUM_SAMPLES_PER_FRAME>
  SimpleAO<NUM_SAMPLES_PER_FRAME>::SimpleAO() 
  {
    ispcEquivalent = ispc::SimpleAO_create(this,NULL,NULL);
  }

  /*! \brief create a material of given type */
  template<int NUM_SAMPLES_PER_FRAME>
  ospray::Material *SimpleAO<NUM_SAMPLES_PER_FRAME>::createMaterial(const char *type) 
  { 
    return new typename SimpleAO<NUM_SAMPLES_PER_FRAME>::Material;
  }

  /*! \brief common function to help printf-debugging */
  template<int NUM_SAMPLES_PER_FRAME>
  std::string SimpleAO<NUM_SAMPLES_PER_FRAME>::toString() const 
  {
    return "ospray::render::SimpleAO"; 
  }
  
  /*! \brief commit the object's outstanding changes (such as changed parameters etc) */
  template<int NUM_SAMPLES_PER_FRAME>
  void SimpleAO<NUM_SAMPLES_PER_FRAME>::commit()
  {
    Renderer::commit();

    bgColor = getParam3f("bgColor",vec3f(1.f));
    ispc::SimpleAO_set(getIE(),
                       (const ispc::vec3f&)bgColor,                           
                       NUM_SAMPLES_PER_FRAME);
  }

  /*! \brief instantiation of SimpleAO that uses 16 AO rays per sample */
  typedef SimpleAO<16> AO16Renderer;
  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
  OSP_REGISTER_RENDERER(AO16Renderer,ao);

  /*! \brief instantiation of SimpleAO that uses 8 AO rays per sample */
  typedef SimpleAO<8>  AO8Renderer;
  OSP_REGISTER_RENDERER(AO8Renderer, ao8);

  /*! \brief instantiation of SimpleAO that uses 4 AO rays per sample */
  typedef SimpleAO<4>  AO4Renderer;
  OSP_REGISTER_RENDERER(AO4Renderer, ao4);

  /*! \brief instantiation of SimpleAO that uses 2 AO rays per sample */
  typedef SimpleAO<2>  AO2Renderer;
  OSP_REGISTER_RENDERER(AO2Renderer, ao2);

  /*! \brief instantiation of SimpleAO that uses 1 AO rays per sample */
  typedef SimpleAO<1>  AO1Renderer;
  OSP_REGISTER_RENDERER(AO1Renderer, ao1);

} // ::ospray

