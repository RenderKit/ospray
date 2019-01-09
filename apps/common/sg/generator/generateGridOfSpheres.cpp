// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "ospcommon/multidim_index_sequence.h"
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

      const auto numSpheres  = dims.product();
      const auto inv_dims    = 1.f / dims;
      const auto min_inv_dim = reduce_min(inv_dims);

      auto sphere_centers = std::make_shared<DataVector3f>();
      sphere_centers->setName("spheres");

      sphere_centers->v.resize(numSpheres);

      index_sequence_3D dims_converter(dims);

      for (size_t i = 0; i < dims_converter.total_indices(); ++i) {
        auto &c = sphere_centers->v[i];
        c = dims_converter.reshape(i) * min_inv_dim;
      }

      spheres_node->add(sphere_centers);

      // spheres attribute nodes

      spheres_node->createChild("radius", "float", min_inv_dim / 5.f);
      spheres_node->createChild("bytes_per_sphere", "int", int(sizeof(vec3f)));

      // finally add to world

      world->add(spheres_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateGridOfSpheres, gridOfSpheres);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateGridOfSpheres, grid_of_spheres);

  }  // ::ospray::sg
}  // ::ospray
