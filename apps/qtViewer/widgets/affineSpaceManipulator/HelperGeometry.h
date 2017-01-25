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

// viewer
#include "sg/common/Common.h"
// std
#include <vector>

namespace ospray {
  namespace viewer {
    using namespace sg;

    //! \brief Helper class for storing tessellated geometry (with
    //! single color per mesh); that can be used to store 3D geometry
    //! for 3D widgets
    struct HelperGeometry {
      //! a quad made up of four vertices with given vertex normal
      struct Quad {
        vec3f vtx[4], nor[4];
      };
      //! a mesh/indexed face set of triangles
      struct Mesh {
        //! base color for this mesh; shared among all vertices
        vec3f color;
        //! vertex array
        std::vector<vec3fa> vertex;
        //! normal array
        std::vector<vec3fa> normal;
        //! triangle/index array
        std::vector<vec3i>  index;
        
        //! \brief add a new quad with given vertices to this triangle mesh
        void addQuad(const affine3f &xfm, const Quad &quad);
      };
    };

    //! \brief A triangle mesh representing a coordinate frame with
    //! three tessellated arrows, with first pointing in X direction
    //! (colored red), the second in Y (colored greed), the third in Z
    //! (colored blue). */
    struct CoordFrameGeometry : public HelperGeometry {
      Mesh arrow[3];
      float shaftThickness;
      float headLength;
      int numSegments;
      
      CoordFrameGeometry();
      void makeArrow(Mesh &mesh,const vec3f &axis);
    };
  } // ::viewer
} // ::ospray

