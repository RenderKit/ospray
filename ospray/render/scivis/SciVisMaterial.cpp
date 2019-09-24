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
#include "SciVisMaterial_ispc.h"

namespace ospray {

  struct SciVisMaterial : public ospray::Material
  {
    SciVisMaterial();
    void commit() override;

   private:
    vec3f Kd;
    float d;
    Ref<Texture2D> map_Kd;
  };

  // SciVisMaterial definitions /////////////////////////////////////////////

  SciVisMaterial::SciVisMaterial()
  {
    ispcEquivalent = ispc::SciVisMaterial_create(this);
  }

  void SciVisMaterial::commit()
  {
    Kd = getParam<vec3f>("color", getParam<vec3f>("kd", getParam<vec3f>("Kd", vec3f(.8f))));
    d  = getParam<float>("d", 1.f);
    map_Kd = (Texture2D *)getParamObject("map_Kd", getParamObject("map_kd"));
    ispc::SciVisMaterial_set(getIE(),
                             (const ispc::vec3f &)Kd,
                             d,
                             map_Kd ? map_Kd->getIE() : nullptr);
  }

  OSP_REGISTER_MATERIAL(scivis, SciVisMaterial, default);

  // NOTE(jda) - support all renderer aliases
  OSP_REGISTER_MATERIAL(sv, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao1, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao2, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao4, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao8, SciVisMaterial, default);
  OSP_REGISTER_MATERIAL(ao16, SciVisMaterial, default);

}  // namespace ospray
