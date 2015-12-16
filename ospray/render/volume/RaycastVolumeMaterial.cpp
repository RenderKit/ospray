// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "RaycastVolumeMaterial.h"
#include "RaycastVolumeRendererMaterial_ispc.h"
#include "ospray/common/Data.h"

namespace ospray {

  RaycastVolumeMaterial::RaycastVolumeMaterial()
  {
    ispcEquivalent = ispc::RaycastVolumeRendererMaterial_create(this);
  }

  void RaycastVolumeMaterial::commit()
  {
    if (ispcEquivalent == NULL)
      ispcEquivalent = ispc::RaycastVolumeRendererMaterial_create(this);

    map_d  = (Texture2D*)getParamObject("map_d", NULL);
    map_Kd = (Texture2D*)getParamObject("map_Kd",
                                        getParamObject("map_kd", NULL));
    map_Ks = (Texture2D*)getParamObject("map_Ks",
                                        getParamObject("map_ks", NULL));
    map_Ns = (Texture2D*)getParamObject("map_Ns",
                                        getParamObject("map_ns", NULL));
    map_Bump = (Texture2D*)getParamObject("map_Bump",
                                          getParamObject("map_bump", NULL));

    d  = getParam1f("d", 1.f);
    Kd = getParam3f("kd", getParam3f("Kd", vec3f(.8f)));
    Ks = getParam3f("ks", getParam3f("Ks", vec3f(0.f)));
    Ns = getParam1f("ns", getParam1f("Ns", 10.f));
    volume = (Volume *)getParamObject("volume", NULL);

    ispc::RaycastVolumeRendererMaterial_set(getIE(),
                               map_d ? map_d->getIE() : NULL,
                               d,
                               map_Kd ? map_Kd->getIE() : NULL,
                               (ispc::vec3f&)Kd,
                               map_Ks ? map_Ks->getIE() : NULL,
                               (ispc::vec3f&)Ks,
                               map_Ns ? map_Ns->getIE() : NULL,
                               Ns,
                               map_Bump != NULL ? map_Bump->getIE() : NULL,
                               volume ? volume->getIE() : NULL);
  }


  OSP_REGISTER_MATERIAL(RaycastVolumeMaterial, RaycastVolumeMaterial);

} // ::ospray
