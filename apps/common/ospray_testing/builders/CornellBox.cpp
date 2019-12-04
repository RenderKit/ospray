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

    struct CornellBox : public detail::Builder
    {
      CornellBox()           = default;
      ~CornellBox() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
      cpp::World buildWorld() const override;
    };

    // quad mesh data
    static std::vector<vec3f> vertices = {
        // Floor
        {1.00f, -1.00f, -1.00f},
        {-1.00f, -1.00f, -1.00f},
        {-1.00f, -1.00f, 1.00f},
        {1.00f, -1.00f, 1.00f},
        // Ceiling
        {1.00f, 1.00f, -1.00f},
        {1.00f, 1.00f, 1.00f},
        {-1.00f, 1.00f, 1.00f},
        {-1.00f, 1.00f, -1.00f},
        // Backwall
        {1.00f, -1.00f, 1.00f},
        {-1.00f, -1.00f, 1.00f},
        {-1.00f, 1.00f, 1.00f},
        {1.00f, 1.00f, 1.00f},
        // RightWall
        {-1.00f, -1.00f, 1.00f},
        {-1.00f, -1.00f, -1.00f},
        {-1.00f, 1.00f, -1.00f},
        {-1.00f, 1.00f, 1.00f},
        // LeftWall
        {1.00f, -1.00f, -1.00f},
        {1.00f, -1.00f, 1.00f},
        {1.00f, 1.00f, 1.00f},
        {1.00f, 1.00f, -1.00f},
        // ShortBox Top Face
        {-0.53f, -0.40f, -0.75f},
        {-0.70f, -0.40f, -0.17f},
        {-0.13f, -0.40f, -0.00f},
        {0.05f, -0.40f, -0.57f},
        // ShortBox Left Face
        {0.05f, -1.00f, -0.57f},
        {0.05f, -0.40f, -0.57f},
        {-0.13f, -0.40f, -0.00f},
        {-0.13f, -1.00f, -0.00f},
        // ShortBox Front Face
        {-0.53f, -1.00f, -0.75f},
        {-0.53f, -0.40f, -0.75f},
        {0.05f, -0.40f, -0.57f},
        {0.05f, -1.00f, -0.57f},
        // ShortBox Right Face
        {-0.70f, -1.00f, -0.17f},
        {-0.70f, -0.40f, -0.17f},
        {-0.53f, -0.40f, -0.75f},
        {-0.53f, -1.00f, -0.75f},
        // ShortBox Back Face
        {-0.13f, -1.00f, -0.00f},
        {-0.13f, -0.40f, -0.00f},
        {-0.70f, -0.40f, -0.17f},
        {-0.70f, -1.00f, -0.17f},
        // ShortBox Bottom Face
        {-0.53f, -1.00f, -0.75f},
        {-0.70f, -1.00f, -0.17f},
        {-0.13f, -1.00f, -0.00f},
        {0.05f, -1.00f, -0.57f},
        // TallBox Top Face
        {0.53f, 0.20f, -0.09f},
        {-0.04f, 0.20f, 0.09f},
        {0.14f, 0.20f, 0.67f},
        {0.71f, 0.20f, 0.49f},
        // TallBox Left Face
        {0.53f, -1.00f, -0.09f},
        {0.53f, 0.20f, -0.09f},
        {0.71f, 0.20f, 0.49f},
        {0.71f, -1.00f, 0.49f},
        // TallBox Front Face
        {0.71f, -1.00f, 0.49f},
        {0.71f, 0.20f, 0.49f},
        {0.14f, 0.20f, 0.67f},
        {0.14f, -1.00f, 0.67f},
        // TallBox Right Face
        {0.14f, -1.00f, 0.67f},
        {0.14f, 0.20f, 0.67f},
        {-0.04f, 0.20f, 0.09f},
        {-0.04f, -1.00f, 0.09f},
        // TallBox Back Face
        {-0.04f, -1.00f, 0.09f},
        {-0.04f, 0.20f, 0.09f},
        {0.53f, 0.20f, -0.09f},
        {0.53f, -1.00f, -0.09f},
        // TallBox Bottom Face
        {0.53f, -1.00f, -0.09f},
        {-0.04f, -1.00f, 0.09f},
        {0.14f, -1.00f, 0.67f},
        {0.71f, -1.00f, 0.49f}};

    static std::vector<vec4ui> indices = {
        {0, 1, 2, 3},      // Floor
        {4, 5, 6, 7},      // Ceiling
        {8, 9, 10, 11},    // Backwall
        {12, 13, 14, 15},  // RightWall
        {16, 17, 18, 19},  // LeftWall
        {20, 21, 22, 23},  // ShortBox Top Face
        {24, 25, 26, 27},  // ShortBox Left Face
        {28, 29, 30, 31},  // ShortBox Front Face
        {32, 33, 34, 35},  // ShortBox Right Face
        {36, 37, 38, 39},  // ShortBox Back Face
        {40, 41, 42, 43},  // ShortBox Bottom Face
        {44, 45, 46, 47},  // TallBox Top Face
        {48, 49, 50, 51},  // TallBox Left Face
        {52, 53, 54, 55},  // TallBox Front Face
        {56, 57, 58, 59},  // TallBox Right Face
        {60, 61, 62, 63},  // TallBox Back Face
        {64, 65, 66, 67}   // TallBox Bottom Face
    };

    static std::vector<vec4f> colors = {
        // Floor
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // Ceiling
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // Backwall
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // RightWall
        {0.140f, 0.450f, 0.091f, 1.0f},
        {0.140f, 0.450f, 0.091f, 1.0f},
        {0.140f, 0.450f, 0.091f, 1.0f},
        {0.140f, 0.450f, 0.091f, 1.0f},
        // LeftWall
        {0.630f, 0.065f, 0.05f, 1.0f},
        {0.630f, 0.065f, 0.05f, 1.0f},
        {0.630f, 0.065f, 0.05f, 1.0f},
        {0.630f, 0.065f, 0.05f, 1.0f},
        // ShortBox Top Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // ShortBox Left Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // ShortBox Front Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // ShortBox Right Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // ShortBox Back Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // ShortBox Bottom Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Top Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Left Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Front Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Right Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Back Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        // TallBox Bottom Face
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f},
        {0.725f, 0.710f, 0.68f, 1.0f}};

    // Inlined definitions ////////////////////////////////////////////////////

    void CornellBox::commit()
    {
      Builder::commit();
      addPlane = false;
    }

    cpp::Group CornellBox::buildGroup() const
    {
      cpp::Geometry quadMesh("mesh");

      quadMesh.setParam("vertex.position", cpp::Data(vertices));
      quadMesh.setParam("vertex.color", cpp::Data(colors));
      quadMesh.setParam("index", cpp::Data(indices));
      quadMesh.commit();

      // create and setup a material
      cpp::Material quadMeshMaterial(rendererType, "OBJMaterial");
      quadMeshMaterial.commit();

      // Put the mesh and material into a model
      cpp::GeometricModel model(quadMesh);
      model.setParam("material", cpp::Data(quadMeshMaterial));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    cpp::World CornellBox::buildWorld() const
    {
      auto world = Builder::buildWorld();

      cpp::Light light("quad");

      light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
      light.setParam("intensity", 47.f);
      light.setParam("position", vec3f(-0.23f, 0.98f, -0.16f));
      light.setParam("edge1", vec3f(0.47f, 0.0f, 0.0f));
      light.setParam("edge2", vec3f(0.0f, 0.0f, 0.38f));

      light.commit();

      world.setParam("light", cpp::Data(light));

      return world;
    }

    OSP_REGISTER_TESTING_BUILDER(CornellBox, cornell_box);

  }  // namespace testing
}  // namespace ospray
