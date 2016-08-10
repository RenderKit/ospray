// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

//ospray
#include "Light.h"
#include "common/Library.h"

//system
#include <map>

namespace ospray {
  typedef Light *(*creatorFct)();
  typedef std::map<std::string, creatorFct> LightRegistry;
  LightRegistry lightRegistry;

  //! Create a new Light object of given type
  Light *Light::createLight(const char *type) {
    LightRegistry::const_iterator it = lightRegistry.find(type);

    if (it != lightRegistry.end())
      return it->second ? (it->second)() : NULL;

    if (ospray::logLevel >= 2)
      std::cout << "#ospray: trying to look up light type '" << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_light__"+std::string(type);

    creatorFct creator = (creatorFct)getSymbol(creatorName);
    lightRegistry[type] = creator;

    if (creator == NULL) {
      if (ospray::logLevel >= 1)
        std::cout << "#ospray: could not find light type '" << type << "'" << std::endl;
      return NULL;
    }
    Light *light = (*creator)();
    light->managedObjectType = OSP_LIGHT;
    return light;
  }

}
