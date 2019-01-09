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

#include "Geometry.h"
// stl
#include <random>
// ospcommon
#include "ospcommon/box.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct RandomSpheres : public Geometry
    {
      RandomSpheres() = default;
      RandomSpheres(int numSpheres) : numSpheres(numSpheres) {}
      ~RandomSpheres() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;

     private:
      int numSpheres{100};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingGeometry RandomSpheres::createGeometry(const std::string &) const
    {
      struct Sphere
      {
        vec3f center;
        float radius;
        vec4f color;
      };

      // create random number distributions for sphere center, radius, and color
      std::random_device rd;
      std::mt19937 gen(rd());

      std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
      std::uniform_real_distribution<float> radiusDistribution(0.05f, 0.15f);
      std::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

      // populate the spheres
      box3f bounds;
      std::vector<Sphere> spheres(numSpheres);

      for (auto &s : spheres) {
        s.center.x = centerDistribution(gen);
        s.center.y = centerDistribution(gen);
        s.center.z = centerDistribution(gen);

        s.radius = radiusDistribution(gen);

        s.color.x = colorDistribution(gen);
        s.color.y = colorDistribution(gen);
        s.color.z = colorDistribution(gen);
        s.color.w = colorDistribution(gen);

        bounds.extend(box3f(s.center - s.radius, s.center + s.radius));
      }

      // create a data object with all the sphere information
      OSPData spheresData =
          ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, spheres.data());

      // create the sphere geometry, and assign attributes
      OSPGeometry spheresGeometry = ospNewGeometry("spheres");

      ospSetData(spheresGeometry, "spheres", spheresData);
      ospSet1i(spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
      ospSet1i(spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
      ospSet1i(spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

      ospSetData(spheresGeometry, "color", spheresData);
      ospSet1i(spheresGeometry, "color_offset", int(offsetof(Sphere, color)));
      ospSet1i(spheresGeometry, "color_format", int(OSP_FLOAT4));
      ospSet1i(spheresGeometry, "color_stride", int(sizeof(Sphere)));

      // create glass material and assign to geometry
      OSPMaterial glassMaterial = ospNewMaterial2("pathtracer", "ThinGlass");
      ospSet1f(glassMaterial, "attenuationDistance", 0.2f);
      ospCommit(glassMaterial);

      ospSetMaterial(spheresGeometry, glassMaterial);

      // commit the spheres geometry
      ospCommit(spheresGeometry);

      // release handles we no longer need
      ospRelease(spheresData);
      ospRelease(glassMaterial);

      OSPTestingGeometry retval;
      retval.geometry = spheresGeometry;
      retval.bounds   = reinterpret_cast<osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(RandomSpheres, spheres);
    OSP_REGISTER_TESTING_GEOMETRY(RandomSpheres, random_spheres);

  }  // namespace testing
}  // namespace ospray
