// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// scene graph common stuff
#include "Common.h"

namespace ospray {
  namespace sg {
    
    //! parse vec3i from std::string (typically an xml-node's content string) 
    vec3i parseVec3i(const std::string &text) 
    { 
      vec3i ret; 
      int rc = sscanf(text.c_str(),"%i %i %i",&ret.x,&ret.y,&ret.z); 
      assert(rc == 3); 
      return ret; 
    }
    
  } // ::ospray::sg
} // ::ospray
