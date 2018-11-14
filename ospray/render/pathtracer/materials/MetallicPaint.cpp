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
#include "MetallicPaint_ispc.h"
#include "math/spectrum.h"
#include "texture/Texture2D.h"

namespace ospray {
  namespace pathtracer {

    struct MetallicPaint : public ospray::Material
    {
      virtual std::string toString() const override
      { return "ospray::pathtracer::MetallicPaint"; }

      MetallicPaint()
      {
        ispcEquivalent = ispc::PathTracer_MetallicPaint_create();
      }

      virtual void commit() override
      {
        const vec3f& color = getParam3f("baseColor",
            getParam3f("color", vec3f(0.8f)));
        Texture2D *map_color = (Texture2D*)getParamObject("map_baseColor",
            getParamObject("map_color"));
        affine2f xform_color = getTextureTransform("map_baseColor")
          * getTextureTransform("map_color");
        const float flakeAmount = getParamf("flakeAmount", 0.3f);
        const vec3f& flakeColor = getParam3f("flakeColor", vec3f(RGB_AL_COLOR));
        const float flakeSpread = getParamf("flakeSpread", 0.5f);
        const float eta = getParamf("eta",  1.5f);

        ispc::PathTracer_MetallicPaint_set(getIE()
            , (const ispc::vec3f&)color
            , map_color ? map_color->getIE() : nullptr
            , (const ispc::AffineSpace2f&)xform_color
            , flakeAmount
            , (const ispc::vec3f&)flakeColor
            , flakeSpread
            , eta
            );
      }
    };

    OSP_REGISTER_MATERIAL(pathtracer, MetallicPaint, MetallicPaint);
    OSP_REGISTER_MATERIAL(pt, MetallicPaint, MetallicPaint);
  }
}
