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
#include "Metal_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Metal : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Metal"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& reflectance
          = getParam3f("reflectance",getParam3f("color",vec3f(1.f))); //vec3f(0.19,0.45,1.5));
        const vec3f& eta
          = getParam3f("eta",vec3f(1.4f)); //vec3f(.4f,0.f,0.f));
        const vec3f& k
          = getParam3f("k",vec3f(1.f)); //3.06,2.4,1.88));
        const float roughness
          = getParamf("roughness",0.01f);
        
        ispcEquivalent = ispc::PathTracer_Metal_create
          ((const ispc::vec3f&)reflectance,
           (const ispc::vec3f&)eta,
           (const ispc::vec3f&)k,
           roughness);
      }
    };

    OSP_REGISTER_MATERIAL(Metal,PathTracer_Metal);
  }
}
