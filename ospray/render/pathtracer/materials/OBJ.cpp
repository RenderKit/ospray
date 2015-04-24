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

#include "ospray/common/Material.h"
#include "ospray/texture/Texture2D.h"
#include "OBJ_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct OBJMaterial : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::OBJMaterial"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() == NULL) {
          Texture2D *map_d  = (Texture2D*)getParamObject("map_d", NULL);
          Texture2D *map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd",  getParamObject("colorMap", NULL)));
          Texture2D *map_Ks = (Texture2D*)getParamObject("map_Ks", getParamObject("map_ks", NULL));
          Texture2D *map_Ns = (Texture2D*)getParamObject("map_Ns", getParamObject("map_ns", NULL));
          Texture2D *map_Bump = (Texture2D*)getParamObject("map_Bump", getParamObject("map_bump", getParamObject("bumpMap", NULL)));

          const float d = getParam1f("d", getParam1f("alpha", 1.f));
          const vec3f Kd = getParam3f("Kd", getParam3f("kd", getParam3f("color", vec3f(0.8f))));
          const vec3f Ks = getParam3f("Ks", getParam3f("ks", vec3f(0.f)));
          const float Ns = getParam1f("Ns", getParam1f("ns", 10.f));

          ispcEquivalent = ispc::PathTracer_OBJ_create
            (map_d ? map_d->getIE() : NULL,
             d,
             map_Kd ? map_Kd->getIE() : NULL,
             (const ispc::vec3f&)Kd,
             map_Ks ? map_Ks->getIE() : NULL,
             (const ispc::vec3f&)Ks,
             map_Ns ? map_Ns->getIE() : NULL,
             Ns,
             map_Bump ? map_Bump->getIE() : NULL);
        }
      }
    };

    OSP_REGISTER_MATERIAL(OBJMaterial,PathTracer_OBJMaterial);
    OSP_REGISTER_MATERIAL(OBJMaterial,PathTracer_default)
  }
}
