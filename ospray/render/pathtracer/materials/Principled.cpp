// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "Principled_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct Principled : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      { return "ospray::pathtracer::Principled"; }

      Principled()
      {
        ispcEquivalent = ispc::PathTracer_Principled_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        Texture2D* map_baseColor = (Texture2D*)getParamObject("map_baseColor");
        affine2f xform_baseColor = getTextureTransform("map_baseColor");
        vec3f baseColor = getParam3f("baseColor", map_baseColor ? vec3f(1.f) : vec3f(0.8f));
        
        Texture2D* map_metallic = (Texture2D*)getParamObject("map_metallic");
        affine2f xform_metallic = getTextureTransform("map_metallic");
        float metallic = getParamf("metallic", map_metallic ? 1.f : 0.f);

        vec3f edgeColor = getParam3f("edgeColor", vec3f(1.f));
        Texture2D* map_edgeColor = (Texture2D*)getParamObject("map_edgeColor");
        affine2f xform_edgeColor = getTextureTransform("map_edgeColor");

        float roughness = getParamf("roughness", 0.1f);
        Texture2D* map_roughness = (Texture2D*)getParamObject("map_roughness");
        affine2f xform_roughness = getTextureTransform("map_roughness");

        ispc::PathTracer_Principled_set(getIE(),
          (const ispc::vec3f&)baseColor, map_baseColor ? map_baseColor->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_baseColor,
          metallic, map_metallic ? map_metallic->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_metallic,
          (const ispc::vec3f&)edgeColor, map_edgeColor ? map_edgeColor->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_edgeColor,
          roughness, map_roughness ? map_roughness->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_roughness);
      }
    };

    OSP_REGISTER_MATERIAL(Principled,PathTracer_Principled);
  }
}
