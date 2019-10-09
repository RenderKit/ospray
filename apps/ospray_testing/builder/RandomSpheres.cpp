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
// ospcommon
#include "ospcommon/math/box.h"

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
      static std::vector<Sphere> spheres(numSpheres);
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
      OSPData positionData =
          ospNewSharedData((char *)spheres.data() + offsetof(Sphere, center),
                           OSP_VEC3F,
                           numSpheres,
                           sizeof(Sphere));
      OSPData radiusData =
          ospNewSharedData((char *)spheres.data() + offsetof(Sphere, radius),
                           OSP_FLOAT,
                           numSpheres,
                           sizeof(Sphere));

      // create the sphere geometry, and assign attributes
      OSPGeometry spheresGeometry = ospNewGeometry("spheres");

      ospSetObject(spheresGeometry, "sphere.position", positionData);
      ospSetObject(spheresGeometry, "sphere.radius", radiusData);

      // commit the spheres geometry
      ospCommit(spheresGeometry);

      OSPGeometricModel model = ospNewGeometricModel(spheresGeometry);

      OSPData colorData = ospNewData(numSpheres, OSP_VEC4F, colors.data());

      ospSetObject(model, "color", colorData);

      // create glass material and assign to geometry
      OSPMaterial glassMaterial =
          ospNewMaterial(rendererType.c_str(), "ThinGlass");
      ospSetFloat(glassMaterial, "attenuationDistance", 0.2f);
      ospCommit(glassMaterial);

      ospSetObjectAsData(model, "material", OSP_MATERIAL, glassMaterial);

      ospCommit(model);

      // release handles we no longer need
      ospRelease(positionData);
      ospRelease(radiusData);
      ospRelease(colorData);
      ospRelease(glassMaterial);

      cpp::Group group;

      ospSetObjectAsData(
          group.handle(), "geometry", OSP_GEOMETRIC_MODEL, model);
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Spheres, random_spheres);

  }  // namespace testing
}  // namespace ospray
