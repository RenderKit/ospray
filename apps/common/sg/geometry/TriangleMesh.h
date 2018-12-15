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

#include "Geometry.h"

namespace ospray {
  namespace sg {

    /*! A Simple Triangle Mesh that stores vertex, normal, texcoord,
        and vertex color in separate arrays */
    struct OSPSG_INTERFACE TriangleMesh : public sg::Geometry
    {
      TriangleMesh();

      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;

      box3f computeBounds() const override;

      void preCommit(RenderContext& ctx) override;

      // NOTE(jda) - Experimental modules may want to make custom triangle
      //             meshes, which mimic everything this ("normal") TriangleMesh
      //             does _except_ for the string type given to ospNewGeometry.
      //             If a custom app assigns a different value to this, then
      //             something other than the default "triangles" geometry
      //             will be created --> in other words, this string is what
      //             is passed to the base Geometry. THIS NEEDS TO BE REVISED
      //             AND IS NOT A PREMENANT SOLUTION!
      static std::string geometry_type;
    };

  } // ::ospray::sg
} // ::ospray


