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

    // Helper type ////////////////////////////////////////////////////////////

    // TODO: generalize to more than just 3D (2D + 4D)
    struct multidim_index_sequence
    {
      multidim_index_sequence(const vec3i &_dims) : dims(_dims) {}

      size_t flatten(const vec3i &coords)
      {
        return coords.x + dims.x * (coords.y + dims.y * coords.z);
      }

      vec3i reshape(size_t i)
      {
        size_t z = i / (dims.x * dims.y);
        i -= (z * dims.x * dims.y);
        size_t y = i / dims.x;
        size_t x = i % dims.x;
        return vec3i(x, y, z);
      }

      size_t total_indices()
      {
        return dims.product();
      }

      // TODO: iterators...

    private:

      vec3ul dims{0};
    };

    // Generator function /////////////////////////////////////////////////////

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

      multidim_index_sequence dims_converter(dims);

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
