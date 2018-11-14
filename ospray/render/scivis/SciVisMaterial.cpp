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

#include "SciVisMaterial.h"
#include "SciVisMaterial_ispc.h"
#include "common/Data.h"

namespace ospray {
  namespace scivis {

    SciVisMaterial::SciVisMaterial()
    {
      ispcEquivalent = ispc::SciVisMaterial_create(this);
    }

    void SciVisMaterial::commit()
    {
      map_d  = (Texture2D*)getParamObject("map_d", nullptr);
      affine2f xform_d  = getTextureTransform("map_d");
      map_Kd = (Texture2D*)getParamObject("map_Kd",
                                          getParamObject("map_kd", nullptr));
      affine2f xform_Kd = getTextureTransform("map_Kd")
        * getTextureTransform("map_kd");
      map_Ks = (Texture2D*)getParamObject("map_Ks",
                                          getParamObject("map_ks", nullptr));
      affine2f xform_Ks = getTextureTransform("map_Ks")
        * getTextureTransform("map_ks");
      map_Ns = (Texture2D*)getParamObject("map_Ns",
                                          getParamObject("map_ns", nullptr));
      affine2f xform_Ns = getTextureTransform("map_Ns")
        * getTextureTransform("map_ns");
      map_Bump = (Texture2D*)getParamObject("map_Bump",
                                            getParamObject("map_bump",nullptr));
      affine2f xform_Bump = getTextureTransform("map_Bump")
        * getTextureTransform("map_bump");
      linear2f rot_Bump = xform_Bump.l.orthogonal().transposed();

      d  = getParam1f("d", 1.f);
      Kd = getParam3f("kd", getParam3f("Kd", vec3f(.8f)));
      Ks = getParam3f("ks", getParam3f("Ks", vec3f(0.f)));
      Ns = getParam1f("ns", getParam1f("Ns", 10.f));
      volume = (Volume *)getParamObject("volume", nullptr);

      ispc::SciVisMaterial_set(getIE(),
                               map_d ? map_d->getIE() : nullptr,
                               (const ispc::AffineSpace2f&)xform_d,
                               d,
                               map_Kd ? map_Kd->getIE() : nullptr,
                               (const ispc::AffineSpace2f&)xform_Kd,
                               (ispc::vec3f&)Kd,
                               map_Ks ? map_Ks->getIE() : nullptr,
                               (const ispc::AffineSpace2f&)xform_Ks,
                               (ispc::vec3f&)Ks,
                               map_Ns ? map_Ns->getIE() : nullptr,
                               (const ispc::AffineSpace2f&)xform_Ns,
                               Ns,
                               map_Bump ? map_Bump->getIE() : nullptr,
                               (const ispc::AffineSpace2f&)xform_Bump,
                               (const ispc::LinearSpace2f&)rot_Bump,
                               volume ? volume->getIE() : nullptr);
    }

    OSP_REGISTER_MATERIAL(scivis, SciVisMaterial, SciVisMaterial);
    OSP_REGISTER_MATERIAL(scivis, SciVisMaterial, OBJMaterial);
    OSP_REGISTER_MATERIAL(scivis, SciVisMaterial, default);

    // NOTE(jda) - support all renderer aliases
    OSP_REGISTER_MATERIAL(rt, SciVisMaterial, default);
    OSP_REGISTER_MATERIAL(raytracer, SciVisMaterial, default);
    OSP_REGISTER_MATERIAL(sv, SciVisMaterial, default);
    OSP_REGISTER_MATERIAL(obj, SciVisMaterial, default);
    OSP_REGISTER_MATERIAL(OBJ, SciVisMaterial, default);
    OSP_REGISTER_MATERIAL(dvr, SciVisMaterial, default);

  } // ::ospray::scivis
} // ::ospray
