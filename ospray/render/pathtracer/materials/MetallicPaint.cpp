// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "common/Material.h"
#include "MetallicPaint_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct MetallicPaint : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::MetallicPaint"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& shadeColor
          = getParam3f("shadeColor",vec3f(0.5,0.42,0.35)); //vec3f(0.19,0.45,1.5));
        const vec3f& glitterColor
          = getParam3f("glitterColor",vec3f(0.5,0.44,0.42)); //3.06,2.4,1.88));

        const float glitterSpread
          = getParamf("glitterSpread",0.f);
        const float eta
          = getParamf("eta",1.45f);
        
        ispcEquivalent = ispc::PathTracer_MetallicPaint_create
          ((const ispc::vec3f&)shadeColor,(const ispc::vec3f&)glitterColor,
           glitterSpread, eta);
      }
    };

    OSP_REGISTER_MATERIAL(MetallicPaint,PathTracer_MetallicPaint);
  }
}
