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

        Texture2D* map_specular = (Texture2D*)getParamObject("map_specular");
        affine2f xform_specular = getTextureTransform("map_specular");
        float specular = getParamf("specular", map_specular ? 1.f : 0.f);

        Texture2D* map_edgeColor = (Texture2D*)getParamObject("map_edgeColor");
        affine2f xform_edgeColor = getTextureTransform("map_edgeColor");
        vec3f edgeColor = getParam3f("edgeColor", vec3f(1.f));
        
        Texture2D* map_roughness = (Texture2D*)getParamObject("map_roughness");
        affine2f xform_roughness = getTextureTransform("map_roughness");
        float roughness = getParamf("roughness", map_roughness ? 1.f : 0.1f);
        
        Texture2D* map_coat = (Texture2D*)getParamObject("map_coat");
        affine2f xform_coat = getTextureTransform("map_coat");
        float coat = getParamf("coat", map_coat ? 1.f : 0.f);

        Texture2D* map_coatColor = (Texture2D*)getParamObject("map_coatColor");
        affine2f xform_coatColor = getTextureTransform("map_coatColor");
        vec3f coatColor = getParam3f("coatColor", vec3f(1.f));

        Texture2D* map_coatThickness = (Texture2D*)getParamObject("map_coatThickness");
        affine2f xform_coatThickness = getTextureTransform("map_coatThickness");
        float coatThickness = getParamf("coatThickness", 1.f);
        
        Texture2D* map_coatRoughness = (Texture2D*)getParamObject("map_coatRoughness");
        affine2f xform_coatRoughness = getTextureTransform("map_coatRoughness");
        float coatRoughness = getParamf("coatRoughness", map_coatRoughness ? 1.f : 0.f);

        ispc::PathTracer_Principled_set(getIE(),
          (const ispc::vec3f&)baseColor, map_baseColor ? map_baseColor->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_baseColor,
          metallic, map_metallic ? map_metallic->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_metallic,
          specular, map_specular ? map_specular->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_specular,
          (const ispc::vec3f&)edgeColor, map_edgeColor ? map_edgeColor->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_edgeColor,
          roughness, map_roughness ? map_roughness->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_roughness,
          coat, map_coat ? map_coat->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_coat,
          (const ispc::vec3f&)coatColor, map_coatColor ? map_coatColor->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_coatColor,
          coatThickness, map_coatThickness ? map_coatThickness->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_coatThickness,
          coatRoughness, map_coatRoughness ? map_coatRoughness->getIE() : nullptr, (const ispc::AffineSpace2f&)xform_coatRoughness);
      }
    };

    OSP_REGISTER_MATERIAL(Principled,PathTracer_Principled);
  }
}
