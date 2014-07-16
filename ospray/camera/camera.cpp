/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray stuff
#include "camera.h"
// embree stuff
#include "../common/library.h"
// stl stuff
#include <map>

namespace ospray {
  typedef Camera *(*creatorFct)();

  std::map<std::string, creatorFct> cameraRegistry;

  Camera *Camera::createCamera(const char *type)
  {
    std::map<std::string, Camera *(*)()>::iterator it = cameraRegistry.find(type);
    if (it != cameraRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up camera type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_camera__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName); //dlsym(RTLD_DEFAULT,creatorName.c_str());
    cameraRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find camera type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }
};

