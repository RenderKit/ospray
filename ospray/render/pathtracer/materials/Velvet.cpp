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
#include "Velvet_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Velvet : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const  override
      { return "ospray::pathtracer::Velvet"; }

      Velvet()
      {
        ispcEquivalent = ispc::PathTracer_Velvet_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        vec3f reflectance              = getParam3f("reflectance",
                                                    vec3f(.4f,0.f,0.f));
        float backScattering           = getParam1f("backScattering",.5f);
        vec3f horizonScatteringColor   = getParam3f("horizonScatteringColor",
                                                    vec3f(.75f,.1f,.1f));
        float horizonScatteringFallOff = getParam1f("horizonScatteringFallOff",10);

        ispc::PathTracer_Velvet_set
          (getIE(), (const ispc::vec3f&)reflectance,(const ispc::vec3f&)horizonScatteringColor,
           horizonScatteringFallOff,backScattering);
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, Velvet, Velvet);
    OSP_REGISTER_MATERIAL(pt, Velvet, Velvet);
  }
}
