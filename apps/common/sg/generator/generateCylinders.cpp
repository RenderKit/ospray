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
#include "ospcommon/multidim_index_sequence.h"
// sg
#include "../common/Data.h"
#include "Generator.h"

namespace ospray {
  namespace sg {

    void generateCylinders(const std::shared_ptr<Node> &world,
                           const std::vector<string_pair> &params)
    {
      auto cylinders_node = createNode("pile_of_cylinders", "Cylinders");

      // get generator parameters

      vec3i dims(10, 10, 10);
      float radius = 2.5f;

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
        } else if (p.first == "radius") {
          radius = std::atof(p.second.c_str());
        } else {
          std::cout << "WARNING: unknown cylinders generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate cylinder data

      const auto numCylinders = dims.product();

      auto cylinder_vertices = std::make_shared<DataVector3f>();

      cylinder_vertices->setName("cylinders");
      cylinder_vertices->v.resize(2 * numCylinders);

      index_sequence_3D dims_converter(dims);

      for (size_t i = 0; i < dims_converter.total_indices(); ++i) {
        auto v1 = dims_converter.reshape(i) * 10;
        auto v2 = v1;

        v1.z = 0.f;

        cylinder_vertices->v[(2 * i) + 0] = v1;
        cylinder_vertices->v[(2 * i) + 1] = v2;
      }

      cylinders_node->add(cylinder_vertices);

      // cylinders attribute nodes

      cylinders_node->createChild("radius", "float", radius);
      cylinders_node->createChild("bytes_per_cylinder",
                                  "int",
                                  int(2 * sizeof(vec3f)),
                                  NodeFlags::gui_readonly);

      // finally add to world

      world->add(cylinders_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateCylinders, cylinders);

  }  // ::ospray::sg
}  // ::ospray
