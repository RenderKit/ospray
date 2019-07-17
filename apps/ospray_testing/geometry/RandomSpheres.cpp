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

    OSPTestingGeometry RandomSpheres::createGeometry(
        const std::string &renderer_type) const
    {
      struct Sphere
      {
        vec3f center;
        float radius;
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
      std::vector<vec4f> colors(numSpheres);

      for (auto &s : spheres) {
        s.center.x = centerDistribution(gen);
        s.center.y = centerDistribution(gen);
        s.center.z = centerDistribution(gen);

        s.radius = radiusDistribution(gen);

        bounds.extend(box3f(s.center - s.radius, s.center + s.radius));
      }

      for (auto &c : colors) {
        c.x = colorDistribution(gen);
        c.y = colorDistribution(gen);
        c.z = colorDistribution(gen);
        c.w = colorDistribution(gen);
      }

      // create a data object with all the sphere information
      OSPData spheresData =
          ospNewData(numSpheres * sizeof(Sphere), OSP_UCHAR, spheres.data());

      // create the sphere geometry, and assign attributes
      OSPGeometry spheresGeometry = ospNewGeometry("spheres");

      ospSetData(spheresGeometry, "spheres", spheresData);
      ospSetInt(spheresGeometry, "bytes_per_sphere", int(sizeof(Sphere)));
      ospSetInt(
          spheresGeometry, "offset_center", int(offsetof(Sphere, center)));
      ospSetInt(
          spheresGeometry, "offset_radius", int(offsetof(Sphere, radius)));

      // commit the spheres geometry
      ospCommit(spheresGeometry);

      OSPGeometricModel model = ospNewGeometricModel(spheresGeometry);

      OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, colors.data());

      ospSetData(model, "prim.color", colorData);

      // create glass material and assign to geometry
      OSPMaterial glassMaterial =
          ospNewMaterial(renderer_type.c_str(), "ThinGlass");
      ospSetFloat(glassMaterial, "attenuationDistance", 0.2f);
      ospCommit(glassMaterial);

      ospSetObject(model, "material", glassMaterial);

      // release handles we no longer need
      ospRelease(spheresData);
      ospRelease(colorData);
      ospRelease(glassMaterial);

      ospCommit(model);

      OSPGroup group = ospNewGroup();
      auto models    = ospNewData(1, OSP_OBJECT, &model);
      ospSetData(group, "geometry", models);
      ospCommit(group);
      ospRelease(models);

      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.geometry = spheresGeometry;
      retval.model    = model;
      retval.group    = group;
      retval.instance = instance;
      retval.bounds   = reinterpret_cast<osp_box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(RandomSpheres, spheres);
    OSP_REGISTER_TESTING_GEOMETRY(RandomSpheres, random_spheres);

  }  // namespace testing
}  // namespace ospray
