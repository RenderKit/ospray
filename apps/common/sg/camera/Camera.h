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

#pragma once

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"

namespace ospray {
  namespace sg {

    /*! a camera node - the generic camera node */
    struct Camera : public sg::Node {
      Camera(const std::string &type) : type(type), ospCamera(NULL) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Camera"; }
      /*! camera type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 

      virtual void create() { 
        if (ospCamera) destroy();
        ospCamera = ospNewCamera(type.c_str());
        commit();
      };
      virtual void commit() {}
      virtual void destroy() {
        if (!ospCamera) return;
        ospRelease(ospCamera);
        ospCamera = 0;
      }

      OSPCamera ospCamera;
    };

  } // ::ospray::sg
} // ::ospray


