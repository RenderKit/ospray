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

#include "../common/Data.h"
#include "Generator.h"

#include <random>

namespace ospray {
  namespace sg {

    void generateRandomQuads(const std::shared_ptr<Node> &world,
                             const std::vector<string_pair> &params)
    {
      auto quads_node = createNode("generated_quads", "QuadMesh");
      //auto quads_node = createNode("generated_quads", "TriangleMesh");
      auto quad_vertices = std::make_shared<DataVector3f>();
      quad_vertices->setName("vertex");
      auto quad_colors = std::make_shared<DataVector3fa>();
      quad_colors->setName("color");
      auto quad_indices = std::make_shared<DataVector1i>();
      quad_indices->setName("index");

      // get generator parameters
      int numQuads = 1;
      float size = 0.05f;

      for (auto &p : params) {
        if (p.first == "numQuads")
          numQuads = std::atoi(p.second.c_str());
        else if (p.first == "size")
          size = std::atof(p.second.c_str());
        else {
          std::cout << "WARNING: unknown quads generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate quad data

      quad_vertices->v.resize(numQuads*4);
      quad_colors->v.resize(numQuads*4);
      quad_indices->v.resize(numQuads*4);

      std::mt19937 rng;
      rng.seed(0);
      std::uniform_real_distribution<float> vert_dist(0, 1.0f);

      static const vec3f delta_x(size, 0   , 0);
      static const vec3f delta_y(0   , size, 0);
      static const vec3fa r(1,0,0);
      static const vec3fa g(0,1,0);
      static const vec3fa b(0,0,1);

      for (int i = 0; i < numQuads * 4; i++) {
        quad_indices->v[i] = i;
      }

      for (int i = 0; i < numQuads * 4; i+=4) {
        auto rv = vec3f(vert_dist(rng), vert_dist(rng), vert_dist(rng));
        auto rc = vec3f(vert_dist(rng), vert_dist(rng), vert_dist(rng));

        quad_vertices->v[i]   = rv;
        quad_vertices->v[i+1] = rv + delta_x;
        quad_vertices->v[i+2] = rv + delta_x + delta_y;
        quad_vertices->v[i+3] = rv + delta_y;

        quad_colors->v[i]    = r+g+b;
        quad_colors->v[i+1]  = r;
        quad_colors->v[i+2]  = g;
        quad_colors->v[i+3]  = b;
      }

      // quads attribute nodes
      quads_node->add(quad_vertices);
      quads_node->add(quad_colors);
      quads_node->add(quad_indices);

      // finally add to world

      world->add(quads_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateRandomQuads, randomQuads);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateRandomQuads, quads);

  }  // ::ospray::sg
}  // ::ospray
