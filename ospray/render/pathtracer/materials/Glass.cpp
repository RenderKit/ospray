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
#include "texture/Texture2D.h"
#include "Glass_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Glass : public ospray::Material {
      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const  override
      { return "ospray::pathtracer::Glass"; }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        if (getIE() == nullptr) {
          ispcEquivalent = ispc::PathTracer_Glass_create();
        }

        const float etaInside = getParamf("etaInside", getParamf("eta", 1.5f));

        const float etaOutside = getParamf("etaOutside", 1.f);

        const vec3f& attenuationColorInside =
          getParam3f("attenuationColorInside",
          getParam3f("attenuationColor",
          getParam3f("color", vec3f(1.f))));

        const vec3f& attenuationColorOutside =
          getParam3f("attenuationColorOutside", vec3f(1.f));

        const float attenuationDistance =
          getParamf("attenuationDistance", getParamf("distance", 1.0f));

        ispc::PathTracer_Glass_set(
          ispcEquivalent,
          etaInside,
          (const ispc::vec3f&)attenuationColorInside,
          etaOutside,
          (const ispc::vec3f&)attenuationColorOutside,
          attenuationDistance);
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, Glass, Glass);
    OSP_REGISTER_MATERIAL(pathtracer, Glass, Dielectric);
    OSP_REGISTER_MATERIAL(pt, Glass, Glass);
    OSP_REGISTER_MATERIAL(pt, Glass, Dielectric);
  }
}
