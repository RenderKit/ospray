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

// ospcommon
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/utility/StringManip.h"
// sg
#include "../common/Data.h"
#include "Generator.h"

namespace ospray {
  namespace sg {

    template <bool asQuads = false>
    void generateCube(const std::shared_ptr<Node> &world,
                      const std::vector<string_pair> &/*params*/)
    {
      auto vertices = std::make_shared<DataVector3f>();
      vertices->setName("vertex");

      vertices->v = containers::AlignedVector<vec3f>{
        vec3f( 1,  1,  1), // 0
        vec3f(-1,  1,  1), // 1
        vec3f(-1, -1,  1), // 2
        vec3f( 1, -1,  1), // 3
        vec3f( 1, -1, -1), // 4
        vec3f( 1,  1, -1), // 5
        vec3f(-1,  1, -1), // 6
        vec3f(-1, -1, -1)  // 7
      };

      auto colors = std::make_shared<DataVector3fa>();
      colors->setName("color");

      static const vec3fa r(1,0,0);
      static const vec3fa g(0,1,0);
      static const vec3fa b(0,0,1);

      colors->v = containers::AlignedVector<vec3fa>{
        0+0+0, // 0
        r+0+0, // 1
        0+g+0, // 2
        0+0+b, // 3
        r+g+0, // 4
        r+0+b, // 5
        0+g+b, // 6
        r+g+b, // 7
      };

      if (asQuads) {
        auto quads_node = createNode("cube", "QuadMesh");

        quads_node->add(vertices);

        auto quad_indices = std::make_shared<DataVector4i>();
        quad_indices->setName("index");

        quad_indices->v = containers::AlignedVector<vec4i>{
          vec4i(0,3,2,1), // +Z
          vec4i(0,1,6,5), // +Y
          vec4i(0,5,4,3), // +X
          vec4i(4,5,6,7), // -Z
          vec4i(2,3,4,7), // -Y
          vec4i(1,2,7,6), // -X
        };

        quads_node->add(quad_indices);
        quads_node->add(colors);

        world->add(quads_node);
      } else {
        auto tris_node = createNode("cube", "TriangleMesh");
        auto tri_indices = std::make_shared<DataVector3i>();
        tri_indices->setName("index");
        tri_indices->v = containers::AlignedVector<vec3i>{
          vec3i(0,3,2), // +Z
          vec3i(2,1,0), // +Z
          vec3i(0,1,6), // +Y
          vec3i(6,5,0), // +Y
          vec3i(0,5,4), // +X
          vec3i(4,3,0), // +X
          vec3i(4,5,6), // -Z
          vec3i(6,7,4), // -Z
          vec3i(2,3,4), // -Y
          vec3i(4,7,2), // -Y
          vec3i(1,2,7), // -X
          vec3i(7,6,1), // -X
        };

        tris_node->add(vertices);
        tris_node->add(tri_indices);
        tris_node->add(colors);

        world->add(tris_node);
      }
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateCube<true>,  cube);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateCube<true>,  cubeQuads);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateCube<false>, cubeTriangles);

  }  // ::ospray::sg
}  // ::ospray
