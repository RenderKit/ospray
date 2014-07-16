/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

//ospray stuff
#include "managed.h"

//embree stuff
#include "common/sys/library.h"

namespace ospray {
  struct Library 
  {
    std::string   name;
    embree::lib_t lib;
  };

  void  loadLibrary(const std::string &name);
  void *getSymbol(const std::string &name);
}
