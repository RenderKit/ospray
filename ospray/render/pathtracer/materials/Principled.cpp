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
        Texture2D* baseColorMap = (Texture2D*)getParamObject("baseColorMap");
        affine2f baseColorXform = getTextureTransform("baseColorMap");
        vec3f baseColor = getParam3f("baseColor", baseColorMap ? vec3f(1.f) : vec3f(0.8f));

        Texture2D* metallicMap = (Texture2D*)getParamObject("metallicMap");
        affine2f metallicXform = getTextureTransform("metallicMap");
        float metallic = getParamf("metallic", metallicMap ? 1.f : 0.f);

        Texture2D* specularMap = (Texture2D*)getParamObject("specularMap");
        affine2f specularXform = getTextureTransform("specularMap");
        float specular = getParamf("specular", specularMap ? 1.f : 0.f);

        Texture2D* edgeColorMap = (Texture2D*)getParamObject("edgeColorMap");
        affine2f edgeColorXform = getTextureTransform("edgeColorMap");
        vec3f edgeColor = getParam3f("edgeColor", vec3f(1.f));

        Texture2D* transmissionMap = (Texture2D*)getParamObject("transmissionMap");
        affine2f transmissionXform = getTextureTransform("transmissionMap");
        float transmission = getParamf("transmission", transmissionMap ? 1.f : 0.f);

        Texture2D* roughnessMap = (Texture2D*)getParamObject("roughnessMap");
        affine2f roughnessXform = getTextureTransform("roughnessMap");
        float roughness = getParamf("roughness", roughnessMap ? 1.f : 0.f);

        Texture2D* normalMap = (Texture2D*)getParamObject("normalMap");
        affine2f normalXform = getTextureTransform("normalMap");
        linear2f normalRot   = normalXform.l.orthogonal().transposed();
        float normalScale = getParamf("normalScale", 1.f);

        Texture2D* coatMap = (Texture2D*)getParamObject("coatMap");
        affine2f coatXform = getTextureTransform("coatMap");
        float coat = getParamf("coat", coatMap ? 1.f : 0.f);

        Texture2D* coatColorMap = (Texture2D*)getParamObject("coatColorMap");
        affine2f coatColorXform = getTextureTransform("coatColorMap");
        vec3f coatColor = getParam3f("coatColor", vec3f(1.f));

        Texture2D* coatThicknessMap = (Texture2D*)getParamObject("coatThicknessMap");
        affine2f coatThicknessXform = getTextureTransform("coatThicknessMap");
        float coatThickness = getParamf("coatThickness", 1.f);

        Texture2D* coatRoughnessMap = (Texture2D*)getParamObject("coatRoughnessMap");
        affine2f coatRoughnessXform = getTextureTransform("coatRoughnessMap");
        float coatRoughness = getParamf("coatRoughness", coatRoughnessMap ? 1.f : 0.f);

        Texture2D* coatNormalMap = (Texture2D*)getParamObject("coatNormalMap");
        affine2f coatNormalXform = getTextureTransform("coatNormalMap");
        linear2f coatNormalRot   = coatNormalXform.l.orthogonal().transposed();
        float coatNormalScale = getParamf("coatNormalScale", 1.f);

        float ior = getParamf("ior", 1.5f);
        vec3f transmissionColor = getParam3f("transmissionColor", vec3f(1.f));
        float transmissionDepth = getParamf("transmissionDepth", 1.f);

        float iorOutside = getParamf("iorOutside", 1.f);
        vec3f transmissionColorOutside = getParam3f("transmissionColorOutside", vec3f(1.f));
        float transmissionDepthOutside = getParamf("transmissionDepthOutside", 1.f);

        ispc::PathTracer_Principled_set(getIE(),
          (const ispc::vec3f&)baseColor, baseColorMap ? baseColorMap->getIE() : nullptr, (const ispc::AffineSpace2f&)baseColorXform,
          metallic, metallicMap ? metallicMap->getIE() : nullptr, (const ispc::AffineSpace2f&)metallicXform,
          specular, specularMap ? specularMap->getIE() : nullptr, (const ispc::AffineSpace2f&)specularXform,
          (const ispc::vec3f&)edgeColor, edgeColorMap ? edgeColorMap->getIE() : nullptr, (const ispc::AffineSpace2f&)edgeColorXform,
          transmission, transmissionMap ? transmissionMap->getIE() : nullptr, (const ispc::AffineSpace2f&)transmissionXform,
          roughness, roughnessMap ? roughnessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)roughnessXform,
          normalMap ? normalMap->getIE() : nullptr, (const ispc::AffineSpace2f&)normalXform, (const ispc::LinearSpace2f&)normalRot, normalScale,
          coat, coatMap ? coatMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatXform,
          (const ispc::vec3f&)coatColor, coatColorMap ? coatColorMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatColorXform,
          coatThickness, coatThicknessMap ? coatThicknessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatThicknessXform,
          coatRoughness, coatRoughnessMap ? coatRoughnessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatRoughnessXform,
          coatNormalMap ? coatNormalMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatNormalXform, (const ispc::LinearSpace2f&)coatNormalRot, coatNormalScale,
          ior,
          (const ispc::vec3f&)transmissionColor,
          transmissionDepth,
          iorOutside,
          (const ispc::vec3f&)transmissionColorOutside,
          transmissionDepthOutside);
      }
    };

    OSP_REGISTER_MATERIAL(Principled,PathTracer_Principled);
  }
}
