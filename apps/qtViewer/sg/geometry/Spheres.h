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

#pragma once

#include "sg/common/Node.h"
#include "sg/common/TransferFunction.h"
#include "sg/geometry/Geometry.h"

namespace ospray {
  namespace sg {

    /*! simple spheres, with all of the key info - position, radius,
        and a int32 type specifier baked into each sphere  */
    struct Spheres : public sg::Geometry {
      struct Sphere { 
        vec3f position;
        float radius;
        uint32 typeID;
        
        // constructor
        Sphere(vec3f position, float radius, uint typeID=0);
        
        // return the bounding box
        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };
      };

      //! constructor
      Spheres() : Geometry("spheres"), ospGeometry(NULL) {};
      
      // return bounding box of all primitives
      virtual box3f getBounds();

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx);

      OSPGeometry         ospGeometry;
      std::vector<Sphere> sphere;
    };

  } // ::ospray::sg
} // ::ospray


