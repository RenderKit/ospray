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
#include "Matte_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Matte : public ospray::Material {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Matte"; }

      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f reflectance = getParam3f("reflectance",vec3f(1.f));
        // const float rcpRoughness = rcpf(roughness);

        ispcEquivalent = ispc::PathTracer_Matte_create
          ((const ispc::vec3f&)reflectance);
      }
    };

    OSP_REGISTER_MATERIAL(Matte,PathTracer_Matte);
  }
}
