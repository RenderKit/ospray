// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "Node.h"
// stl
#include <stdexcept>

namespace ospray {
  namespace sg {

    /*! \brief scene graph run time error abstraction. 

      \detailed we derive from std::runtime_error to allow apps to
      just catch those errors if they're not explicitly interested in
      where it came from */
    struct OSPSG_INTERFACE RuntimeError : public std::runtime_error {
      RuntimeError(const std::string &err) : std::runtime_error("#osp:sg: "+err) {}
    };

  }
}
