/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "objmaterial.h"
#include "objmaterial_ispc.h"
#include "../../common/data.h"

namespace ospray {
  namespace obj {

    //extern "C" void ispc_OBJMaterial_create(void *cppE);
    //    extern "C" void ispc_OBJMaterial_set(

    //! \brief commit the material's parameters
    void OBJMaterial::commit()
    {
      if (ispcEquivalent == NULL)
        ispcEquivalent = ispc::OBJMaterial_create(this);

#if 0
      Ref<Data> textureArrayData = getParamData("textureArray",NULL);
      int32 num_textures = textureArrayData ? textureArrayData->numItems : 0;

      if(textureArrayData && num_textures > 0) {
        Texture2D **textures = (ospray::Texture2D**)textureArrayData->data;
        int map_d_idx    = getParam1i("map_d", -1);
        int map_Kd_idx   = getParam1i("map_Kd", getParam1i("map_kd",-1));
        int map_Ks_idx   = getParam1i("map_Ks", getParam1i("map_ks",-1));
        int map_Ns_idx   = getParam1i("map_Ns", getParam1i("map_ns",-1));
        int map_Bump_idx = getParam1i("map_Bump", 
                                      getParam1i("map_bump",-1));

        map_d =     map_d_idx    >= 0 ? textures[map_d_idx] : NULL;
        
        map_Kd =    map_Kd_idx   >= 0 ? textures[map_Kd_idx] : NULL;
        map_Ks =    map_Ks_idx   >= 0 ? textures[map_Ks_idx] : NULL;
        map_Ns =    map_Ns_idx   >= 0 ? textures[map_Ns_idx] : NULL;
        map_Bump =  map_Bump_idx >= 0 ? textures[map_Bump_idx] : NULL;
      } else {
        map_d = map_Kd = map_Ks = map_Ns = map_Bump = NULL;
      }
#else
      Data* map_d_data = getParamData("map_d", NULL);
      Data* map_Kd_data = getParamData("map_Kd", getParamData("map_kd", NULL));
      Data* map_Ks_data = getParamData("map_Ks", getParamData("map_ks", NULL));
      Data* map_Ns_data = getParamData("map_Ns", getParamData("map_ns", NULL));
      Data* map_Bump_data = getParamData("map_Bump", getParamData("map_bump", NULL));

      map_d  = map_d_data ? (Texture2D*)map_d_data->data : NULL;
      map_Kd = map_Kd_data ? (Texture2D*)map_Kd_data->data : NULL;
      map_Ks = map_Ks_data ? (Texture2D*)map_Ks_data->data : NULL;
      map_Ns = map_Ns_data ? (Texture2D*)map_Ns_data->data : NULL;
      map_Bump = map_Bump_data ? (Texture2D*)map_Bump_data->data : NULL;
#endif

      d  = getParam1f("d", 1.f);
      Kd = getParam3f("kd", getParam3f("Kd", vec3f(.8f)));
      Ks = getParam3f("ks", getParam3f("Ks", vec3f(0.f)));
      Ns = getParam1f("ns", getParam1f("Ns", 0.f));

      ispc::OBJMaterial_set(getIE(),
                            map_d ? map_d->getIE() : NULL,
                            d,
                            map_Kd ? map_Kd->getIE() : NULL,
                            (ispc::vec3f&)Kd,
                            map_Ks ? map_Ks->getIE() : NULL,
                            (ispc::vec3f&)Ks,
                            map_Ns ? map_Ns->getIE() : NULL,
                            Ns,
                            map_Bump != NULL ? map_Bump->getIE() : NULL );
    }

    OBJMaterial::~OBJMaterial()
    {
      if (getIE() != NULL)
        ispc::OBJMaterial_destroy(getIE());
    }
    
    OSP_REGISTER_MATERIAL(OBJMaterial,OBJMaterial);
  }
}
