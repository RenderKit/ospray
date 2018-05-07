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

    void generateRandomSpheres(const std::shared_ptr<Node> &world,
                               const std::vector<string_pair> &params)
    {
      auto spheres_node = createNode("generated_spheres", "Spheres");

      // get generator parameters

      const float sceneLowerBound = 0.f;
      const float sceneUpperBound = 1.f;

      int numSpheres = 1e6;
      float radius   = 0.001f;

      for (auto &p : params) {
        if (p.first == "numSpheres")
          numSpheres = std::atoi(p.second.c_str());
        else if (p.first == "radius")
          radius = std::atof(p.second.c_str());
        else {
          std::cout << "WARNING: unknown spheres generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate sphere data

      auto spheres = std::make_shared<DataVector3f>();
      spheres->setName("spheres");

      spheres->v.resize(numSpheres);

      std::mt19937 rng;
      rng.seed(0);
      std::uniform_real_distribution<float> vert_dist(sceneLowerBound,
                                                      sceneUpperBound);

      for (int i = 0; i < numSpheres; ++i) {
        auto &s = spheres->v[i];

        s.x = vert_dist(rng);
        s.y = vert_dist(rng);
        s.z = vert_dist(rng);
      }

      spheres_node->add(spheres);

      // spheres attribute nodes

      spheres_node->createChild("radius", "float", radius);
      spheres_node->createChild("bytes_per_sphere", "int", int(sizeof(vec3f)));

      // finally add to world

      world->add(spheres_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateRandomSpheres, randomSpheres);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateRandomSpheres, spheres);

  }  // ::ospray::sg
}  // ::ospray
