/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

//ospray
#include "light.h"
#include "ospray/common/library.h"

//system
#include <map>

namespace ospray {
  typedef Light *(*creatorFct)();
  typedef std::map<std::string, creatorFct> LightRegistry;
  LightRegistry lightRegistry;

  //! Create a new Light object of given type
  Light *Light::createLight(const char *type) {
    LightRegistry::const_iterator it = lightRegistry.find(type);

    if (it != lightRegistry.end()) return it->second ? (it->second)() : NULL;

    if (ospray::logLevel >= 2)
      std::cout << "#ospray: trying to look up light type '" << type << "'for the first time" << std::endl;

    std::string creatorName = "ospray_create_light__"+std::string(type);

    creatorFct creator = (creatorFct)getSymbol(creatorName);
    lightRegistry[type] = creator;

    if (creator == NULL) {
      if (ospray::logLevel >= 1)
        std::cout << "#ospray: could not find light type '" << type << "'" << std::endl;
      return NULL;
    }
    Light *light = (*creator)();  light->managedObjectType = OSP_LIGHT;
    return(light);
  }

}
