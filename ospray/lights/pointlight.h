/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "light.h"

namespace ospray {

  //! Base class for PointLight objects, inherits from Light
  struct PointLight : public Light {
    public:

      //!Construct a new point light.
      PointLight();

      //! toString is used to aid in printf debugging
      virtual std::string toString() const { return "ospray::PointLight"; }

      //! Copy understood parameters into member parameters
      virtual void commit();

      vec3f position;               //! world-space position of the light
      vec3f color;                  //! RGB color of the light
      float range;                  //! range after which light has no effect

  };

}
