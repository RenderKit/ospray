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
#include "common/Data.h"
#include "texture/Texture2D.h"
#include "math/spectrum.h"
#include "Alloy_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Alloy : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      { return "ospray::pathtracer::Alloy"; }

      Alloy()
      {
        ispcEquivalent = ispc::PathTracer_Alloy_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        const vec3f& color = getParam3f("color", vec3f(0.9f));
        Texture2D *map_color = (Texture2D*)getParamObject("map_color");
        affine2f xform_color = getTextureTransform("map_color");

        const vec3f& edgeColor = getParam3f("edgeColor", vec3f(1.f));
        Texture2D *map_edgeColor = (Texture2D*)getParamObject("map_edgeColor");
        affine2f xform_edgeColor = getTextureTransform("map_edgeColor");

        const float roughness = getParamf("roughness", 0.1f);
        Texture2D *map_roughness = (Texture2D*)getParamObject("map_roughness");
        affine2f xform_roughness = getTextureTransform("map_roughness");

        ispc::PathTracer_Alloy_set(getIE()
            , (const ispc::vec3f&)color
            , map_color ? map_color->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_color
            , (const ispc::vec3f&)edgeColor
            , map_edgeColor ? map_edgeColor->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_edgeColor
            , roughness
            , map_roughness ? map_roughness->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_roughness
            );
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, Alloy, Alloy);
    OSP_REGISTER_MATERIAL(pt, Alloy, Alloy);
  }
}
