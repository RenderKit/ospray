// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

    struct MetallicPaint : public ospray::Material
    {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      { return "ospray::pathtracer::MetallicPaint"; }

      MetallicPaint()
      {
        ispcEquivalent = ispc::PathTracer_MetallicPaint_create();
      }
      
      //! \brief commit the material's parameters
      virtual void commit() override
      {
        const vec3f& shadeColor
          = getParam3f("shadeColor",vec3f(0.5,0.42,0.35)); //vec3f(0.19,0.45,1.5));

        const float glitterSpread
          = getParamf("glitterSpread",0.f);
        const float eta
          = getParamf("eta",1.45f);
        
        ispc::PathTracer_MetallicPaint_set(getIE(),
          (const ispc::vec3f&)shadeColor,
           glitterSpread, eta);
      }
    };

    OSP_REGISTER_MATERIAL(MetallicPaint,PathTracer_MetallicPaint);
  }
}
