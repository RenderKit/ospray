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

    void generateSpheres(const std::shared_ptr<Node> &world,
                         const std::vector<string_pair> &params)
    {
      auto spheres_node = createNode("generated_spheres", "Spheres");

      // generate spheres themselves

      struct Sphere
      {
        vec3f org;
        int colorID {0};
      };

      const float sceneLowerBound = 0.f;
      const float sceneUpperBound = 1.f;

      int numSpheres = 1e6;
      int numColors  = 5;
      float radius   = 0.002f;

      for (auto &p : params) {
        if (p.first == "numSpheres")
          numSpheres = std::atoi(p.second.c_str());
        else if (p.first == "numColors")
          numColors = std::atoi(p.second.c_str());
        else if (p.first == "radius")
          radius = std::atof(p.second.c_str());
        else {
          std::cout << "WARNING: unknown spheres generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      auto *spheres = new Sphere[numSpheres];

      std::mt19937 rng;
      rng.seed(std::random_device()());
      std::uniform_real_distribution<float> vert_dist(sceneLowerBound,
                                                      sceneUpperBound);

      std::uniform_int_distribution<int> colorID_dist(0, numColors);

      for (int i = 0; i < numSpheres; ++i) {
        auto &s = spheres[i];

        s.org.x   = vert_dist(rng);
        s.org.y   = vert_dist(rng);
        s.org.z   = vert_dist(rng);
        s.colorID = colorID_dist(rng);
      }

      // create colors

      auto *colors = new vec3f[numColors];

      std::uniform_real_distribution<float> color_dist(0.f, 1.f);

      for (int i = 0; i < numColors; ++i) {
        auto &c = colors[i];

        c.x = color_dist(rng);
        c.y = color_dist(rng);
        c.z = color_dist(rng);
      }

      // create data nodes

      auto sphere_data =
          std::make_shared<DataArrayRAW>((byte_t*)spheres,
                                         numSpheres * sizeof(Sphere));

      auto color_data = std::make_shared<DataArray3f>(colors, numColors);

      sphere_data->setName("spheres");
      color_data->setName("color");

      spheres_node->add(sphere_data);
      spheres_node->add(color_data);

      // spheres attribute nodes

      spheres_node->createChild("offset_colorID", "int", int(sizeof(vec3f)));
      spheres_node->createChild("radius", "float", radius);
      spheres_node->createChild("bytes_per_sphere", "int", int(sizeof(Sphere)));

      // finally add to world

      world->add(spheres_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateSpheres, spheres);

  }  // ::ospray::sg
}  // ::ospray
