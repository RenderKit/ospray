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
#include "CarPaint_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct CarPaint : public ospray::Material
    {
      //! \brief common function to help printf-debugging
      /*! Every derived class should overrride this! */
      virtual std::string toString() const override
      { return "ospray::pathtracer::CarPaint"; }

      CarPaint()
      {
        ispcEquivalent = ispc::PathTracer_CarPaint_create();
      }

      //! \brief commit the material's parameters
      virtual void commit() override
      {
        Texture2D* baseColorMap = (Texture2D*)getParamObject("baseColorMap");
        affine2f baseColorXform = getTextureTransform("baseColorMap");
        vec3f baseColor = getParam3f("baseColor", baseColorMap ? vec3f(1.f) : vec3f(0.8f));

        Texture2D* baseRoughnessMap = (Texture2D*)getParamObject("baseRoughnessMap");
        affine2f baseRoughnessXform = getTextureTransform("baseRoughnessMap");
        float baseRoughness = getParamf("baseRoughness", baseRoughnessMap ? 1.f : 0.f);

        Texture2D* flakeScaleMap = (Texture2D*)getParamObject("flakeScaleMap");
        affine2f flakeScaleXform = getTextureTransform("flakeScaleMap");
        float flakeScale = getParamf("flakeScale", flakeScaleMap ? 1.f : 100.f);

        Texture2D* flakeDensityMap = (Texture2D*)getParamObject("flakeDensityMap");
        affine2f flakeDensityXform = getTextureTransform("flakeDensityMap");
        float flakeDensity = getParamf("flakeDensity", flakeDensityMap ? 1.f : 1.f);

        Texture2D* flakeSpreadMap = (Texture2D*)getParamObject("flakeSpreadMap");
        affine2f flakeSpreadXform = getTextureTransform("flakeSpreadMap");
        float flakeSpread = getParamf("flakeSpread", flakeSpreadMap ? 1.f : 0.2f);

        Texture2D* flakeJitterMap = (Texture2D*)getParamObject("flakeJitterMap");
        affine2f flakeJitterXform = getTextureTransform("flakeJitterMap");
        float flakeJitter = getParamf("flakeJitter", flakeJitterMap ? 1.f : 0.75f);

        Texture2D* flakeRoughnessMap = (Texture2D*)getParamObject("flakeRoughnessMap");
        affine2f flakeRoughnessXform = getTextureTransform("flakeRoughnessMap");
        float flakeRoughness = getParamf("flakeRoughness", flakeRoughnessMap ? 1.f : 0.3f);

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

        ispc::PathTracer_CarPaint_set(getIE(),
          (const ispc::vec3f&)baseColor, baseColorMap ? baseColorMap->getIE() : nullptr, (const ispc::AffineSpace2f&)baseColorXform,
          baseRoughness, baseRoughnessMap ? baseRoughnessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)baseRoughnessXform,
          flakeScale, flakeScaleMap ? flakeScaleMap->getIE() : nullptr, (const ispc::AffineSpace2f&)flakeScaleXform,
          flakeDensity, flakeDensityMap ? flakeDensityMap->getIE() : nullptr, (const ispc::AffineSpace2f&)flakeDensityXform,
          flakeSpread, flakeSpreadMap ? flakeSpreadMap->getIE() : nullptr, (const ispc::AffineSpace2f&)flakeSpreadXform,
          flakeJitter, flakeJitterMap ? flakeJitterMap->getIE() : nullptr, (const ispc::AffineSpace2f&)flakeJitterXform,
          flakeRoughness, flakeRoughnessMap ? flakeRoughnessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)flakeRoughnessXform,
          coat, coatMap ? coatMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatXform,
          (const ispc::vec3f&)coatColor, coatColorMap ? coatColorMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatColorXform,
          coatThickness, coatThicknessMap ? coatThicknessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatThicknessXform,
          coatRoughness, coatRoughnessMap ? coatRoughnessMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatRoughnessXform,
          coatNormalMap ? coatNormalMap->getIE() : nullptr, (const ispc::AffineSpace2f&)coatNormalXform, (const ispc::LinearSpace2f&)coatNormalRot, coatNormalScale);
      }
    };

    OSP_REGISTER_MATERIAL(CarPaint,PathTracer_CarPaint);
  }
}
