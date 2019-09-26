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

#include <vector>

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct CornellBox : public Geometry
    {
      CornellBox()           = default;
      ~CornellBox() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    // quad mesh data
    static vec3f vertices[] = {
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

    static vec4i indices[] = {
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

    static vec4f colors[] = {
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

    OSPTestingGeometry CornellBox::createGeometry(
        const std::string &renderer_type) const
    {
      box3f bounds(vec3f(-1.0f), vec3f(1.0f));

      OSPGeometry quadMesh = ospNewGeometry("quads");

      OSPData quadVerts   = ospNewData(17 * 4, OSP_VEC3F, vertices);
      OSPData quadColors  = ospNewData(17 * 4, OSP_VEC4F, colors);
      OSPData quadIndices = ospNewData(17, OSP_VEC4UI, indices);

      ospCommit(quadVerts);
      ospCommit(quadColors);
      ospCommit(quadIndices);

      ospSetObject(quadMesh, "vertex.position", quadVerts);
      ospSetObject(quadMesh, "vertex.color", quadColors);
      ospSetObject(quadMesh, "index", quadIndices);

      ospCommit(quadMesh);
      ospRelease(quadVerts);
      ospRelease(quadColors);
      ospRelease(quadIndices);

      // create and setup a material
      OSPMaterial quadMeshMaterial =
          ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      ospCommit(quadMeshMaterial);

      // Put the mesh and material into a model
      OSPGeometricModel quadMeshModel = ospNewGeometricModel(quadMesh);
      ospSetObject(quadMeshModel, "material", quadMeshMaterial);
      ospCommit(quadMeshModel);
      ospRelease(quadMeshMaterial);

      // put the model into a group (collection of models)
      OSPGroup group = ospNewGroup();
      ospSetObjectAsData(group, "geometry", OSP_GEOMETRIC_MODEL, quadMeshModel);
      ospCommit(group);

      // put the group into an instance (give the group a world transform)
      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.geometry = quadMesh;
      retval.model    = quadMeshModel;
      retval.group    = group;
      retval.instance = instance;

      std::memcpy(&retval.bounds, &bounds, sizeof(bounds));

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(CornellBox, cornell_box);

  }  // namespace testing
}  // namespace ospray
