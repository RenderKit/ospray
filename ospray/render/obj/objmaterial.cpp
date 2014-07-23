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

    //! \brief commit the material's parameters
    void OBJMaterial::commit()
    {
      if (ispcEquivalent == NULL)
        ispcEquivalent = ispc::OBJMaterial_create(this);

      map_d  = (Texture2D*)getParamObject("map_d", NULL);
      map_Kd = (Texture2D*)getParamObject("map_Kd", getParamObject("map_kd", NULL));
      map_Ks = (Texture2D*)getParamObject("map_Ks", getParamObject("map_ks", NULL));
      map_Ns = (Texture2D*)getParamObject("map_Ns", getParamObject("map_ns", NULL));
      map_Bump = (Texture2D*)getParamObject("map_Bump", getParamObject("map_bump", NULL));

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
