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
#include "Dielectric_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Dielectric : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Dielectric"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& transmission
          = getParam3f("transmission",vec3f(1.f));
        const vec3f& transmissionOutside
          = getParam3f("transmissionOutside",vec3f(1.f));

        const float etaInside
          = getParamf("etaInside",1.4f);
        const float etaOutside
          = getParamf("etaOutside",1.f);
        
        ispcEquivalent = ispc::PathTracer_Dielectric_create
          (etaOutside,(const ispc::vec3f&)transmissionOutside,
           etaInside,(const ispc::vec3f&)transmission);
      }
    };

    OSP_REGISTER_MATERIAL(Dielectric,PathTracer_Dielectric);
  }
}
