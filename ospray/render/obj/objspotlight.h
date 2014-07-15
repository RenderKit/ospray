/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ospray/lights/spotlight.h"

namespace ospray {
  namespace obj {
    //!< OBJ renderer specific implementation of SpotLight
    struct OBJSpotLight : public SpotLight {
      OBJSpotLight();

      //! \brief common function to help printf-debugging
      virtual std::string toString() const { return "ospray::objrenderer::OBJPointLight"; }

      //! \brief commit the light's parameters
      virtual void commit();

      //! destructor, to clean up
      virtual ~OBJSpotLight();

      //Attenuation will be calculated as 1/( constantAttenuation + linearAttenuation * distance + quadraticAttenuation * distance * distance )
      float constantAttenuation;    //! Constant light attenuation
      float linearAttenuation;      //! Linear light attenuation
      float quadraticAttenuation;   //! Quadratic light attenuation
    };
  }
}
