// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// ospray::sg
#include "Module.h"
#include "../common/RuntimeError.h"
// ospray
#include "ospcommon/common.h"
// std
#include <set>

/*! \file sg/module/Module.cpp Defines the interface for writing
    ospray::sg modules */

namespace ospray {
  namespace sg {

    /*! allows a user of the scene graph (e.g., a model viewer) to
      load a scene graph module */
    void loadModule(const std::string &moduleName)
    {
      static std::set<std::string> alreadyLoaded;
      if (alreadyLoaded.find(moduleName) != alreadyLoaded.end()) return;

      alreadyLoaded.insert(moduleName);

      const std::string libName = "ospray_module_sg_"+moduleName;
      const std::string symName = "ospray_sg_"+moduleName+"_init";
      
      ospcommon::loadLibrary(libName);
      void *sym = ospcommon::getSymbol(symName);
      if (!sym)
        throw sg::RuntimeError("could not load module '" + moduleName
                               + "' (symbol '" + symName + "' not found)");

      void (*initFct)() = (void (*)())sym;
      initFct();
    }

  }
}


