// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "common/Material.h"
#include "texture/Texture2D.h"
// ispc
#include "SimpleAOMaterial_ispc.h"

namespace ospray {

  struct SimpleAOMaterial : public ospray::Material
  {
    SimpleAOMaterial();
    void commit() override;

   private:
    vec3f Kd;
    Ref<Texture2D> map_Kd;
  };

  // SimpleAOMaterial definitions /////////////////////////////////////////////

  SimpleAOMaterial::SimpleAOMaterial()
  {
    ispcEquivalent = ispc::SimpleAOMaterial_create(this);
  }

  void SimpleAOMaterial::commit()
  {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(.8f))));
    map_Kd =
        (Texture2D *)getParamObject("map_Kd", getParamObject("map_kd"));
    ispc::SimpleAOMaterial_set(getIE(),
                               (const ispc::vec3f &)Kd,
                               map_Kd ? map_Kd->getIE() : nullptr);
  }

  OSP_REGISTER_MATERIAL(ao, SimpleAOMaterial, default);

  // NOTE(jda) - support all renderer aliases
  OSP_REGISTER_MATERIAL(ao1, SimpleAOMaterial, default);
  OSP_REGISTER_MATERIAL(ao2, SimpleAOMaterial, default);
  OSP_REGISTER_MATERIAL(ao4, SimpleAOMaterial, default);
  OSP_REGISTER_MATERIAL(ao8, SimpleAOMaterial, default);
  OSP_REGISTER_MATERIAL(ao16, SimpleAOMaterial, default);

}  // namespace ospray
