/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "PointLight.h"

namespace ospray {
  //!Construct a new PointLight object
  PointLight::PointLight()
    : position(0.f, 0.f, 0.f)
    , color(1.f, 1.f, 1.f)
    , range(-1.f)
  {
  }

  //!Commit parameters understood by the base PointLight class
  void PointLight::commit() {
    position = getParam3f("position", vec3f(0.f, 0.f, 0.f));
    color    = getParam3f("color", vec3f(1.f, 1.f, 1.f));
    range    = getParam1f("range", -1.f);
  }
}
