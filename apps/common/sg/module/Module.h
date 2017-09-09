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

#pragma once

// ospray::sg
#include "../common/World.h"

/*! \file sg/module/Module.h Defines the interface for writing
    ospray::sg modules */

namespace ospray {
  namespace sg {

    /*! allows a user of the scene graph (e.g., a model viewer) to
        load a scene graph module */
    void OSPSG_INTERFACE loadModule(const std::string &moduleName);

    /*! use this macro in a loadable module to properly define this
        module to the scene graph. Make sure to follow this
        declaration with a function body that does the actual
        initialization of this module. OLD NAMING SCHEME */
#define OSPRAY_SG_DECLARE_MODULE(moduleName)            \
    extern "C" void ospray_sg_##moduleName##_init()


    /*! use this macro in a loadable module to properly define this
        module to the scene graph. Make sure to follow this
        declaration with a function body that does the actual
        initialization of this module.

        Note: NEW NAMING SCHEME to make it consistent with
        OSP_SG_REGISTER_NODE macro */
#define OSP_SG_REGISTER_MODULE(moduleName)            \
    extern "C" void ospray_sg_##moduleName##_init()

  }
}


