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
#include "Importer.h"

#include <random>

namespace ospray {
  namespace sg {

    void generateBasicVolume(const std::shared_ptr<Node> &world,
                               const std::vector<string_pair> &params)
    {
      auto volume_node = createNode("basic_volume", "StructuredVolume");

      // get generator parameters

      vec3i dims(100, 100, 100);

#if 0
      int numSpheres = 1e6;
      float radius   = 0.002f;

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
#else
      (void)params;
#endif

      // generate volume data

      auto numVoxels = dims.product();

      float *voxels = new float[numVoxels];

      for (int i = 0; i < numVoxels; ++i)
        voxels[i] = float(i);

      // create data nodes

      auto voxel_data = std::make_shared<DataArray1f>(voxels, numVoxels);

      voxel_data->setName("voxelData");

      volume_node->add(voxel_data);

      // volume attributes

      volume_node->child("voxelType")  = std::string("float");
      volume_node->child("dimensions") = dims;

      // add volume to world

      world->add(volume_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateBasicVolume, basic_volume);
    OSPSG_REGISTER_GENERATE_FUNCTION(generateBasicVolume, volume);

  }  // ::ospray::sg
}  // ::ospray
