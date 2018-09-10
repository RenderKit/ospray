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
#include "Mix_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct MixMaterial : public ospray::Material
    {
      virtual std::string toString() const override
      { return "ospray::pathtracer::MixMaterial"; }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        if (getIE() == nullptr)
          ispcEquivalent = ispc::PathTracer_Mix_create();

        float factor = getParam1f("factor", 0.5f);
        Texture2D *map_factor  = (Texture2D*)getParamObject("map_factor", nullptr);
        affine2f xform_factor  = getTextureTransform("map_factor");
        ospray::Material* mat1 = (ospray::Material*)getParamObject("material1", nullptr);
        ospray::Material* mat2 = (ospray::Material*)getParamObject("material2", nullptr);

        ispc::PathTracer_Mix_set(ispcEquivalent
            , factor
            , map_factor ? map_factor->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_factor
            , mat1 ? mat1->getIE() : nullptr
            , mat2 ? mat2->getIE() : nullptr
            );
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, MixMaterial, MixMaterial);
    OSP_REGISTER_MATERIAL(pt, MixMaterial, MixMaterial);
  }
}
