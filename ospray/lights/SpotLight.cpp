/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "SpotLight.h"

namespace ospray {
  //!< Constructor for SpotLight. Placed in protected section to make SpotLight act as an abstract base class.
  SpotLight::SpotLight()
    : position(0.f, 0.f, 0.f)
    , direction(1.f, 1.f, 1.f)
    , color(1.f, 1.f, 1.f)
    , range(-1.f)
    , halfAngle(-1.f)
  {}

  //!< Copy understood parameters into class members
  void SpotLight::commit() {
    position  = getParam3f("position", vec3f(0.f, 0.f, 0.f));
    direction = getParam3f("direction", vec3f(1.f, 1.f, 1.f));
    color = getParam3f("color", vec3f(1.f, 1.f, 1.f));
    range = getParam1f("range", -1.f);
    halfAngle = getParam1f("halfAngle", -1.f);
  }
}
