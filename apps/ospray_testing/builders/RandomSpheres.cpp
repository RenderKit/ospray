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

#include "Builder.h"
#include "ospray_testing.h"
// stl
#include <random>

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Spheres : public detail::Builder
    {
      Spheres()           = default;
      ~Spheres() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;

     private:
      // Data //

      int numSpheres{100};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Spheres::commit()
    {
      Builder::commit();

      numSpheres = getParam<int>("numSpheres", 100);
    }

    cpp::Group Spheres::buildGroup() const
    {
      std::vector<vec3f> s_center(numSpheres);
      std::vector<float> s_radius(numSpheres);
      std::vector<vec4f> s_colors(numSpheres);

      // create random number distributions for sphere center, radius, and color
      std::mt19937 gen(randomSeed);

      std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
      std::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);
      std::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

      // populate the spheres

      for (auto &center : s_center) {
        center.x = centerDistribution(gen);
        center.y = centerDistribution(gen);
        center.z = centerDistribution(gen);
      }

      for (auto &radius : s_radius)
        radius = radiusDistribution(gen);

      for (auto &c : s_colors) {
        c.x = colorDistribution(gen);
        c.y = colorDistribution(gen);
        c.z = colorDistribution(gen);
        c.w = colorDistribution(gen);
      }

      // create the sphere geometry, and assign attributes
      cpp::Geometry spheresGeometry("spheres");

      spheresGeometry.setParam("sphere.position", cpp::Data(s_center));
      spheresGeometry.setParam("sphere.radius", cpp::Data(s_radius));
      spheresGeometry.commit();

      cpp::GeometricModel model(spheresGeometry);

      model.setParam("color", cpp::Data(s_colors));

      // create glass material and assign to geometry
      cpp::Material glassMaterial(rendererType.c_str(), "ThinGlass");
      glassMaterial.setParam("attenuationDistance", 0.2f);
      glassMaterial.commit();

      model.setParam("material", cpp::Data(glassMaterial));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Spheres, random_spheres);

  }  // namespace testing
}  // namespace ospray
