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

#include "common/Material.h"
#include "Volumetric_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct VolumetricMaterial : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const  override
      { return "ospray::pathtracer::VolumetricMaterial"; }

      //! \brief commit the material's parameters
      virtual void commit()  override
      {
        if (getIE() == nullptr)
          ispcEquivalent = ispc::PathTracer_Volumetric_create();

        vec3f albedo     = getParam3f("albedo",     vec3f(1.0f));
        float meanCosine = getParam1f("meanCosine", 0.f);

        ispc::PathTracer_Volumetric_set(ispcEquivalent, (const ispc::vec3f&)albedo, meanCosine);
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, VolumetricMaterial, VolumetricMaterial);
    OSP_REGISTER_MATERIAL(pt, VolumetricMaterial, VolumetricMaterial);
  }
}
