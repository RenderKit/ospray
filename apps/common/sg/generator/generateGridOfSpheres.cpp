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

    void generateGridOfSpheres(const std::shared_ptr<Node> &world,
                               const std::vector<string_pair> &params)
    {
      auto spheres_node = createNode("grid_of_spheres", "Spheres");

      // get generator parameters

      vec3i dims(100, 100, 100);

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 3) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxYxZ'"
                      << std::endl;
            continue;
          }

          dims = vec3i(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()),
                       std::atoi(string_dims[2].c_str()));
        } else {
          std::cout << "WARNING: unknown spheres generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate sphere data

      const auto numSpheres = dims.product();
      const auto inv_dims = 1.f / dims;

      auto sphere_centers = std::make_shared<DataVector3f>();
      sphere_centers->setName("spheres");

      sphere_centers->v.resize(numSpheres);

      vec3f current(0.f);

      for (int i = 0; i < numSpheres; ++i) {
        auto &c = sphere_centers->v[i];

        c = current;

        current.x += inv_dims.x;
        if (current.x > 1.f) {
          current.x = 0.f;
          current.y += inv_dims.x;
          if (current.y > 1.f) {
            current.y = 0.f;
            current.z += inv_dims.x;
            if (current.z > 1.f)
              current.z = 0.f;
          }
        }
      }

      spheres_node->add(sphere_centers);

      // spheres attribute nodes

      spheres_node->createChild("radius", "float", inv_dims.x / 5.f);
      spheres_node->createChild("bytes_per_sphere", "int", int(sizeof(vec3f)));

      // finally add to world

      world->add(spheres_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateGridOfSpheres, gridOfSpheres);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateGridOfSpheres, grid_of_spheres);

  }  // ::ospray::sg
}  // ::ospray
