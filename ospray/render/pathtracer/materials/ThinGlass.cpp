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
#include "ThinGlass_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct ThinGlass : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const  override
      { return "ospray::pathtracer::ThinGlass"; }

      ThinGlass()
      {
        ispcEquivalent = ispc::PathTracer_ThinGlass_create();
      }

      //! \brief commit the material's parameters
      virtual void commit()  override
      {
        const float eta = getParamf("eta", 1.5f);
        const vec3f& attenuationColor =
          getParam3f("attenuationColor",
              getParam3f("transmission",
                getParam3f("color", vec3f(1.f))));
        const float attenuationDistance =
          getParamf("attenuationDistance", 1.f);
        const float thickness = getParamf("thickness", 1.f);

        Texture2D *map_attenuationColor =
          (Texture2D*)getParamObject("map_attenuationColor",
              getParamObject("colorMap", nullptr));
        affine2f xform_attenuationColor =
          getTextureTransform("map_attenuationColor")
          * getTextureTransform("colorMap");

        ispc::PathTracer_ThinGlass_set(getIE()
            , eta
            , (const ispc::vec3f&)attenuationColor
            , map_attenuationColor ? map_attenuationColor->getIE() : nullptr,
              (const ispc::AffineSpace2f&)xform_attenuationColor
            , attenuationDistance
            , thickness
            );
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, ThinGlass, ThinGlass);
    OSP_REGISTER_MATERIAL(pt, ThinGlass, ThinGlass);
  }
}
