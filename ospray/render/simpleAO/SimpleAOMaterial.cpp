// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "SimpleAOMaterial.h"
#include "SimpleAOMaterial_ispc.h"

namespace ospray {
  namespace simpleao {

    //! Constructor
    Material::Material()
    {
      ispcEquivalent = ispc::SimpleAOMaterial_create(this);
    }

    void Material::commit()
    {
      Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
      map_Kd = (Texture2D*)getParamObject("map_Kd",
                                          getParamObject("map_kd", NULL));
      ispc::SimpleAOMaterial_set(getIE(),
                                 (const ispc::vec3f&)Kd,
                                 map_Kd.ptr != NULL ? map_Kd->getIE() : NULL);
    }

    OSP_REGISTER_MATERIAL(ao,   simpleao::Material, default);

    // NOTE(jda) - support all renderer aliases
    OSP_REGISTER_MATERIAL(ao1,  simpleao::Material, default);
    OSP_REGISTER_MATERIAL(ao2,  simpleao::Material, default);
    OSP_REGISTER_MATERIAL(ao4,  simpleao::Material, default);
    OSP_REGISTER_MATERIAL(ao8,  simpleao::Material, default);
    OSP_REGISTER_MATERIAL(ao16, simpleao::Material, default);

  }//namespace ospray::simpleao
}//namespace ospray
