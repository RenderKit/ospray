// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "OBJ_ispc.h"

namespace ospray {
  namespace pathtracer {

    struct OBJMaterial : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::OBJMaterial"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() == NULL)
          ispcEquivalent = ispc::PathTracer_OBJ_create();
       
          Texture2D *map_d  = (Texture2D*)getParamObject("map_d", NULL);
          affine2f xform_d  = getTextureTransform("map_d");
          Texture2D *map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd",  getParamObject("colorMap", NULL)));
          affine2f xform_Kd = getTextureTransform("map_Kd") * getTextureTransform("map_kd") * getTextureTransform("colorMap"); 
          Texture2D *map_Ks = (Texture2D*)getParamObject("map_Ks", getParamObject("map_ks", NULL));
          affine2f xform_Ks = getTextureTransform("map_Ks") * getTextureTransform("map_ks"); 
          Texture2D *map_Ns = (Texture2D*)getParamObject("map_Ns", getParamObject("map_ns", NULL));
          affine2f xform_Ns = getTextureTransform("map_Ns") * getTextureTransform("map_ns"); 
          Texture2D *map_Bump = (Texture2D*)getParamObject("map_Bump", getParamObject("map_bump", getParamObject("normalMap", getParamObject("bumpMap", NULL))));
          affine2f xform_Bump = getTextureTransform("map_Bump") * getTextureTransform("map_bump") * getTextureTransform("normalMap") * getTextureTransform("BumpMap"); 
          linear2f rot_Bump   = xform_Bump.l.orthogonal().transposed();

          const float d = getParam1f("d", getParam1f("alpha", 1.f));
          const vec3f Kd = getParam3f("Kd", getParam3f("kd", getParam3f("color", vec3f(0.8f))));
          const vec3f Ks = getParam3f("Ks", getParam3f("ks", vec3f(0.f)));
          const float Ns = getParam1f("Ns", getParam1f("ns", 10.f));
          const vec3f Tf = getParam3f("Tf", getParam3f("tf", vec3f(0.0f)));

          if (reduce_max(Kd + Ks + Tf) > 1.0)
            std::cout << "#osp:PT: warning: OBJ material produces energy (Kd + Ks + Tf must be <= 1)" << std::endl;

          ispc::PathTracer_OBJ_set(ispcEquivalent,
             map_d ? map_d->getIE() : NULL, (const ispc::AffineSpace2f&)xform_d,
             d,
             map_Kd ? map_Kd->getIE() : NULL, (const ispc::AffineSpace2f&)xform_Kd,
             (const ispc::vec3f&)Kd,
             map_Ks ? map_Ks->getIE() : NULL, (const ispc::AffineSpace2f&)xform_Ks,
             (const ispc::vec3f&)Ks,
             map_Ns ? map_Ns->getIE() : NULL, (const ispc::AffineSpace2f&)xform_Ns,
             Ns,
             (const ispc::vec3f&)Tf,
             map_Bump ? map_Bump->getIE() : NULL, (const ispc::AffineSpace2f&)xform_Bump,
             (const ispc::LinearSpace2f&)rot_Bump);
      }
    };

    OSP_REGISTER_MATERIAL(OBJMaterial,PathTracer_OBJMaterial);
    OSP_REGISTER_MATERIAL(OBJMaterial,PathTracer_default)
  }
}
