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
#include "Luminous_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Luminous : public ospray::Material
    {
      Luminous()
      {
        ispcEquivalent = ispc::PathTracer_Luminous_create();
      }

      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      {
        return "ospray::pathtracer::Luminous";
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        const vec3f radiance = getParam3f("color", vec3f(1.f)) *
                               getParam1f("intensity", 1.f);
        const float transparency = getParam1f("transparency", 0.f);

        ispc::PathTracer_Luminous_set(getIE()
            , (const ispc::vec3f&)radiance
            , transparency
            );
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, Luminous, Luminous);
    OSP_REGISTER_MATERIAL(pt, Luminous, Luminous);
  }
}
