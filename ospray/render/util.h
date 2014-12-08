/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \file render/util.h Defines some utility functions shaared by different shading codes */

#include "common/OSPCommon.h"

namespace ospray {

  //! generates a "random" color from an int.
  inline vec3f makeRandomColor(const int i)
  {
    const int mx = 13*17*43;
    const int my = 11*29;
    const int mz = 7*23*63;
    const uint32 g = (i * (3*5*127)+12312314);
    return vec3f((g % mx)*(1.f/(mx-1)),
                 (g % my)*(1.f/(my-1)),
                 (g % mz)*(1.f/(mz-1)));
  }

}
