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

#include "Volume.h"
// stl
#include <vector>
// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct SimpleUnStructuredVolume : public Volume
    {
      SimpleUnStructuredVolume()           = default;
      ~SimpleUnStructuredVolume() override = default;

      OSPTestingVolume createVolume() const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingVolume SimpleUnStructuredVolume::createVolume() const
    {
      // create an unstructured volume object
      OSPVolume volume = ospNewVolume("unstructured_volume");

      // define hexahedron parameters
      const float hSize = .5f;
      const float hX = -.5f, hY = -.5f, hZ = 0.f;

      // define wedge parameters
      const float wSize = .5f;
      const float wX = .5f, wY = -.5f, wZ = 0.f;

      // define tetrahedron parameters
      const float tSize = .5f;
      const float tX = .5f, tY = .5f, tZ = 0.f;

      // define vertex positions, duplicated vertices
      // that can be shared between cells are commented out
      std::vector<vec3f> vertices = {
          // hexahedron bottom quad
          {-hSize + hX, -hSize + hY, hSize + hZ},   // 0
          {hSize + hX, -hSize + hY, hSize + hZ},    // 1
          {hSize + hX, -hSize + hY, -hSize + hZ},   // 2
          {-hSize + hX, -hSize + hY, -hSize + hZ},  // 3
                                                    // hexahedron top quad
          {-hSize + hX, hSize + hY, hSize + hZ},    // 4
          {hSize + hX, hSize + hY, hSize + hZ},     // 5
          {hSize + hX, hSize + hY, -hSize + hZ},    // 6
          {-hSize + hX, hSize + hY, -hSize + hZ},   // 7

          // wedge bottom triangle, sharing 2 hexahedron vertices
          // { -wSize + wX, -wSize + wY, -wSize + wZ }, // 1
          {wSize + wX, -wSize + wY, 0.f + wZ},  // 8
          // { -wSize + wX, -wSize + wY,  wSize + wZ }, // 2
          // wedge top triangle, sharing 2 hexahedron vertices
          // { -wSize + wX,  wSize + wY, -wSize + wZ }, // 5
          {wSize + wX, wSize + wY, 0.f + wZ},  // 9
          // { -wSize + wX,  wSize + wY,  wSize + wZ }, // 6

          // tetrahedron, sharing 3 vertices
          // { -tSize + tX, -tSize + tY, -tSize + tZ }, // 5
          // {  tSize + tX, -tSize + tY,    0.f + tZ }, // 9
          // { -tSize + tX, -tSize + tY,  tSize + tZ }, // 6
          {0.f + tX, tSize + tY, 0.f + tZ}  // 10
      };

      // define cell field value
      std::vector<float> cellFields = {0.2f, 5.f, 9.8f};

      // shape cells by defining indices
      std::vector<vec4i> indices = {// hexahedron
                                    {0, 1, 2, 3},
                                    {4, 5, 6, 7},

                                    // wedge
                                    {-2, -2, 1, 8},
                                    {2, 5, 9, 6},

                                    // tetrahedron
                                    {-1, -1, -1, -1},
                                    {5, 9, 6, 10}};

      // create data objects
      OSPData verticesData =
          ospNewData(vertices.size(), OSP_FLOAT3, vertices.data());
      OSPData cellFieldsData =
          ospNewData(cellFields.size(), OSP_FLOAT, cellFields.data());
      OSPData indicesData =
          ospNewData(indices.size(), OSP_INT4, indices.data());

      // calculate bounds
      box3f bounds;
      std::for_each(vertices.begin(), vertices.end(), [&](vec3f &v) {
        bounds.extend(v);
      });

      // set data objects for volume object
      ospSetData(volume, "vertices", verticesData);
      ospSetData(volume, "cellField", cellFieldsData);
      ospSetData(volume, "indices", indicesData);

      // release handlers that go out of scope here
      ospRelease(verticesData);
      ospRelease(cellFieldsData);
      ospRelease(indicesData);

      // create OSPRay objects and return results
      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = osp::vec2f{0.f, 10.f};
      retval.bounds     = reinterpret_cast<const osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(SimpleUnStructuredVolume, unstructured_volume);
    OSP_REGISTER_TESTING_VOLUME(SimpleUnStructuredVolume,
                                simple_unstructured_volume);

  }  // namespace testing
}  // namespace ospray
