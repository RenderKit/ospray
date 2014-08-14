/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "model.h"

namespace ospray {
  namespace particle {
    //! parse given uintah-format timestep.xml file, and return in a model
    Model *parse__Uintah_timestep_xml(const std::string &s);
  }
}
