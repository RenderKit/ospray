/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "OBJPointLight.h"
#include "OBJPointLight_ispc.h"

namespace ospray {
  namespace obj {

    //! Create a new OBJPointLight object and ispc equivalent
    OBJPointLight::OBJPointLight() {
      ispcEquivalent = ispc::OBJPointLight_create(this);
    }

    //! Commit parameters to class members. Pass data on to ispc side object.
    void OBJPointLight::commit() {

      //commit inherited params
      PointLight::commit();

      constantAttenuation = getParam1f("attenuation.constant", 0.f);
      linearAttenuation = getParam1f("attenuation.linear", 0.f);
      quadraticAttenuation = getParam1f("attenuation.quadratic", 0.f);

      ispc::OBJPointLight_set(getIE(),
                              (ispc::vec3f&)position,
                              (ispc::vec3f&)color,
                              range,
                              constantAttenuation,
                              linearAttenuation,
                              quadraticAttenuation);
    }

    //! Destroy an OBJPointLight object
    OBJPointLight::~OBJPointLight(){}

    //Register the light type
    OSP_REGISTER_LIGHT(OBJPointLight, OBJ_PointLight);

  }
}
