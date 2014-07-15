/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// ospray
#include "texture.h"
#include "common/library.h"

//stl
#include <map>

namespace ospray {
  
  typedef Texture *(*creatorFct)();
  std::map<std::string, creatorFct> textureRegistry;

  /*! \brief creates an abstract texture class of a given type
   *
   * The respective texture type must be a registered texture type
   * in either ospray proper or an already loaded module. For texture
   * types specified in special modules, make sure to call
   * ospLoadModule first.
   */
  Texture *Texture::createTexture(const char *identifier )
  {
    char type[strlen(identifier)+2];
    strcpy(type,identifier);
    for (char *s = type; *s; ++s)
      if (*s == '-') *s = '_';

    std::map<std::string, Texture *(*)()>::iterator it = textureRegistry.find(type);
    if (it != textureRegistry.end())
      return it->second ? (it->second)() : NULL;

    if (ospray::logLevel >= 2)
      std::cout << "#ospray: trying to look up texture type '"
        << type << "' for the first time..." << std::endl;

    std::string creatorName = "ospray_create_texture__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName);
    textureRegistry[type] = creator;
    if (creator = NULL) {
      if (ospray::logLevel >= 1)
        std::cout << "#ospray: could not find texture type '" << type << "'" << std::endl;
      return NULL;
    }
    return (*creator)();
  }

}
