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

    struct Cylinders : public detail::Builder
    {
      Cylinders()           = default;
      ~Cylinders() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;

     private:
      vec2i numPatches{9};
      int numCylindersPerPatch{32};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Cylinders::commit()
    {
      Builder::commit();

      numPatches           = getParam<vec2i>("numPathces", vec2i(9));
      numCylindersPerPatch = getParam<int>("numCylindersPerPatch", 32);
    }

    cpp::Group Cylinders::buildGroup() const
    {
      // Create Cylinder struct
      struct Cylinder
      {
        vec3f startVertex;
        vec3f endVertex;
        float radius;
      };

      std::random_device rd;
      std::mt19937 gen(rd());

      std::uniform_real_distribution<float> startDelta(-0.015625f, 0.015625f);
      std::uniform_real_distribution<float> endDelta(-0.125f, 0.125f);
      std::uniform_real_distribution<float> radiusDistribution(0.001f, 0.002f);
      std::uniform_real_distribution<float> colorDelta(-0.1f, 0.1f);

      // Set up our patches of grass
      int numCylinders = numPatches.x * numPatches.y * numCylindersPerPatch;

      // populate the cylinders
      static std::vector<Cylinder> cylinders(numCylinders);
      std::vector<vec3f> c_startVertex(numCylinders);
      std::vector<vec3f> c_endVertex(numCylinders);
      std::vector<float> c_radius(numCylinders);
      std::vector<vec4f> c_colors(numCylinders);

      for (int pz = 0; pz < numPatches.y; pz++) {
        for (int px = 0; px < numPatches.x; px++) {
          for (int ps = 0; ps < numCylindersPerPatch; ps++) {
            int idx = (pz * numPatches.x + px) * numCylindersPerPatch + ps;

            auto &startVertex = c_startVertex.at(idx);
            auto &endVertex   = c_endVertex.at(idx);
            auto &radius      = c_radius.at(idx);

            startVertex.x =
                (4.0f / numPatches.x) * (px + 0.5f) - 2.0f + startDelta(gen);
            startVertex.y = -1.0f;
            startVertex.z =
                (4.0f / numPatches.y) * (pz + 0.5f) - 2.0f + startDelta(gen);

            endVertex.x =
                (4.0f / numPatches.x) * (px + 0.5f) - 2.0f + endDelta(gen);
            endVertex.y = -0.5f + endDelta(gen);
            endVertex.z =
                (4.0f / numPatches.y) * (pz + 0.5f) - 2.0f + endDelta(gen);

            radius = radiusDistribution(gen);
          }
        }
      }

      // Make all cylinders a greenish hue, with some luma variance
      for (auto &c : c_colors) {
        c.x = 0.2f + colorDelta(gen);
        c.y = 0.6f + colorDelta(gen);
        c.z = 0.1f + colorDelta(gen);
        c.w = 1.0f;
      }

      // create the cylinder geometry, and assign attributes
      cpp::Geometry cylindersGeometry("cylinders");

      cylindersGeometry.setParam(
          "cylinder.position0",
          cpp::Data(c_startVertex.size(), OSP_VEC3F, c_startVertex.data()));
      cylindersGeometry.setParam(
          "cylinder.position1",
          cpp::Data(c_endVertex.size(), OSP_VEC3F, c_endVertex.data()));
      cylindersGeometry.setParam(
          "cylinder.radius",
          cpp::Data(c_radius.size(), OSP_VEC3F, c_radius.data()));

      // commit the cylinders geometry
      cylindersGeometry.commit();

      // Add the color data to the cylinders
      cpp::GeometricModel model(cylindersGeometry);
      model.setParam("color",
                     cpp::Data(c_colors.size(), OSP_VEC4F, c_colors.data()));

      // create obj material and assign to cylinders model
      cpp::Material material(rendererType, "OBJMaterial");
      material.commit();

      model.setParam("material", cpp::Data(material));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Cylinders, cylinders);

  }  // namespace testing
}  // namespace ospray
