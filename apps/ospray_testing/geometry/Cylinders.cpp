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
// ospcommon
#include "ospcommon/math/box.h"

#include <random>
#include <vector>

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Cylinders : public Geometry
    {
      Cylinders()           = default;
      ~Cylinders() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingGeometry Cylinders::createGeometry(
        const std::string &renderer_type) const
    {
      box3f bounds(vec3f(-1.0f), vec3f(1.0f));

      // Create Cylinder struct
      struct Cylinder
      {
        vec3f startVertex;
        vec3f endVertex;
        float radius;
      };

      // create random number distributions for cylinder start, end, radius,
      // color
      std::random_device rd;
      std::mt19937 gen(rd());

      std::uniform_real_distribution<float> startDelta(-0.015625f, 0.015625f);
      std::uniform_real_distribution<float> endDelta(-0.125f, 0.125f);
      std::uniform_real_distribution<float> radiusDistribution(0.001f, 0.002f);
      std::uniform_real_distribution<float> colorDelta(-0.1f, 0.1f);

      // Set up our patches of grass
      vec2i numPatches         = vec2i{9, 9};
      int numCylindersPerPatch = 32;
      int numCylinders = numPatches.x * numPatches.y * numCylindersPerPatch;

      // populate the cylinders
      static std::vector<Cylinder> cylinders(numCylinders);
      std::vector<vec4f> colors(numCylinders);  // TODO put in Cylinder as well

      for (int pz = 0; pz < numPatches.y; pz++) {
        for (int px = 0; px < numPatches.x; px++) {
          for (int ps = 0; ps < numCylindersPerPatch; ps++) {
            Cylinder &s = cylinders.at(
                (pz * numPatches.x + px) * numCylindersPerPatch + ps);
            s.startVertex.x =
                (4.0f / numPatches.x) * (px + 0.5f) - 2.0f + startDelta(gen);
            s.startVertex.y = -1.0f;
            s.startVertex.z =
                (4.0f / numPatches.y) * (pz + 0.5f) - 2.0f + startDelta(gen);

            s.endVertex.x =
                (4.0f / numPatches.x) * (px + 0.5f) - 2.0f + endDelta(gen);
            s.endVertex.y = -0.5f + endDelta(gen);
            s.endVertex.z =
                (4.0f / numPatches.y) * (pz + 0.5f) - 2.0f + endDelta(gen);

            s.radius = radiusDistribution(gen);
          }
        }
      }

      // Make all cylinders a greenish hue, with some luma variance
      for (auto &c : colors) {
        c.x = 0.2f + colorDelta(gen);
        c.y = 0.6f + colorDelta(gen);
        c.z = 0.1f + colorDelta(gen);
        c.w = 1.0f;
      }

      // create a data objects with all the cylinder information
      OSPData startVertexData = ospNewSharedData(
          (char *)cylinders.data() + offsetof(Cylinder, startVertex),
          OSP_VEC3F,
          numCylinders,
          sizeof(Cylinder));
      OSPData endVertexData = ospNewSharedData(
          (char *)cylinders.data() + offsetof(Cylinder, endVertex),
          OSP_VEC3F,
          numCylinders,
          sizeof(Cylinder));
      OSPData radiusData = ospNewSharedData(
          (char *)cylinders.data() + offsetof(Cylinder, radius),
          OSP_FLOAT,
          numCylinders,
          sizeof(Cylinder));

      // create the cylinder geometry, and assign attributes
      OSPGeometry cylindersGeometry = ospNewGeometry("cylinders");

      ospSetObject(cylindersGeometry, "cylinder.position0", startVertexData);
      ospSetObject(cylindersGeometry, "cylinder.position1", endVertexData);
      ospSetObject(cylindersGeometry, "cylinder.radius", radiusData);

      // commit the cylinders geometry
      ospCommit(cylindersGeometry);

      // Add the color data to the cylinders
      OSPGeometricModel model = ospNewGeometricModel(cylindersGeometry);
      OSPData cylindersColor =
          ospNewData(numCylinders, OSP_VEC4F, colors.data());
      ospSetObject(model, "prim.color", cylindersColor);

      // create obj material and assign to cylinders model
      OSPMaterial objMaterial =
          ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      // ospSetVec3f(objMaterial, "Ks", 0.8f, 0.8f, 0.8f);
      ospCommit(objMaterial);

      ospSetObject(model, "material", objMaterial);
      ospCommit(model);

      OSPGroup group = ospNewGroup();
      ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, model);
      ospCommit(group);

      // Codify cylinders into an instance
      OSPInstance cylindersInstance = ospNewInstance(group);
      ospCommit(cylindersInstance);

      OSPTestingGeometry retval;
      retval.geometry = cylindersGeometry;
      retval.model    = model;
      retval.group    = group;
      retval.instance = cylindersInstance;

      std::memcpy(&retval.bounds, &bounds, sizeof(bounds));

      // release handles we no longer need
      ospRelease(startVertexData);
      ospRelease(endVertexData);
      ospRelease(radiusData);
      ospRelease(cylindersColor);
      ospRelease(objMaterial);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(Cylinders, cylinders);

  }  // namespace testing
}  // namespace ospray
