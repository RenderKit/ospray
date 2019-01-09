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
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/StringManip.h"
// sg
#include "../common/Data.h"
#include "Generator.h"

#include <random>

namespace ospray {
  namespace sg {

    void generateBasicVolume(const std::shared_ptr<Node> &world,
                               const std::vector<string_pair> &params)
    {
      auto volume_node = createNode("basic_volume", "StructuredVolume");

      // get generator parameters

      vec3l dims(100, 100, 100);

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 3) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxYxZ'"
                      << std::endl;
            continue;
          }

          dims = vec3l(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()),
                       std::atoi(string_dims[2].c_str()));
        } else {
          std::cout << "WARNING: unknown spheres generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate volume data

      auto numVoxels = dims.product();

      float *voxels = new float[numVoxels];

      const float r = 0.03f * reduce_max(dims);
      const float R = 0.08f * reduce_max(dims);

      tasking::parallel_for(dims.z, [&](int64_t z) {
        for (int y = 0; y < dims.y; ++y) {
          for (int x = 0; x < dims.x; ++x) {
            const float X = x - dims.x / 2;
            const float Y = y - dims.y / 2;
            const float Z = z - dims.z / 2;

            float d = (R - std::sqrt(X*X + Y*Y));
            voxels[dims.x * dims.y * z + dims.x * y + x] = r*r - d*d - Z*Z;
          }
        }
      });

      // create data nodes

      auto voxel_data = std::make_shared<DataArray1f>(voxels, numVoxels);

      voxel_data->setName("voxelData");

      volume_node->add(voxel_data);

      // volume attributes

      volume_node->child("voxelType")  = std::string("float");
      volume_node->child("dimensions") = vec3i(dims);

      // add volume to world

      world->add(volume_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateBasicVolume, basic_volume);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateBasicVolume, volume);

  }  // ::ospray::sg
}  // ::ospray
