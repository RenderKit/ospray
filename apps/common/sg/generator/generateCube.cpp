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
#include "ospcommon/utility/StringManip.h"
// sg
#include "../common/Data.h"
#include "Generator.h"

namespace ospray {
  namespace sg {

    void generateCube(const std::shared_ptr<Node> &world,
                      const std::vector<string_pair> &/*params*/)
    {
      auto quads_node = createNode("cube", "QuadMesh");

      // generate cube using quads

      auto quad_vertices = std::make_shared<DataVector3f>();
      quad_vertices->setName("vertex");

      quad_vertices->v = std::vector<vec3f>{
        vec3f( 1,  1,  1), // 0
        vec3f(-1,  1,  1), // 1
        vec3f(-1, -1,  1), // 2
        vec3f( 1, -1,  1), // 3
        vec3f( 1, -1, -1), // 4
        vec3f( 1,  1, -1), // 5
        vec3f(-1,  1, -1), // 6
        vec3f(-1, -1, -1)  // 7
      };

      quads_node->add(quad_vertices);

#if 1
      auto quad_indices = std::make_shared<DataVector4i>();
      quad_indices->setName("index");

      quad_indices->v = std::vector<vec4i>{
		vec4i(0,1,2,3), // +Z
		vec4i(0,1,6,5), // +Y
		vec4i(0,3,4,5), // +X
		vec4i(4,5,6,7), // -Z
		vec4i(2,3,4,7), // -Y
		vec4i(1,2,7,6), // -X
      };
#else
      auto quad_indices = std::make_shared<DataVector1i>();
      quad_indices->setName("index");

      quad_indices->v = std::vector<int>{
        0,1,3,2,
        0,1,5,4,
        0,3,7,4,
        2,1,5,6,
        2,3,7,6,
        4,5,6,7
      };
#endif

      quads_node->add(quad_indices);

      // finally add to world

      world->add(quads_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateCube, cube);

  }  // ::ospray::sg
}  // ::ospray
