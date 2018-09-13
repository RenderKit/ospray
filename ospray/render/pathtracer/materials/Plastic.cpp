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

#include "common/Material.h"
#include "Plastic_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Plastic : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const  override
      { return "ospray::pathtracer::Plastic"; }

      Plastic()
      {
        ispcEquivalent = ispc::PathTracer_Plastic_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        const vec3f pigmentColor = getParam3f("pigmentColor",vec3f(1.f));
        const float eta          = getParamf("eta",1.4f);
        const float roughness    = getParamf("roughness",0.01f);

        ispc::PathTracer_Plastic_set
          (getIE(), (const ispc::vec3f&)pigmentColor,eta,roughness);
      }
    };

    OSP_REGISTER_MATERIAL(pt, Plastic, Plastic);
  }
}
