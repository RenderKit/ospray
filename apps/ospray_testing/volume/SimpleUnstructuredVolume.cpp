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
// unstructured cell types
#include "ospray/OSPUnstructured.h"

// values defined per-cell or per-vertex
//#define VALUES_PER_CELL

// separate cells or cells sharing vertices
//#define SHARED_VERTICES

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
      const float hSize = .4f;
      const float hX = -.5f, hY = -.5f, hZ = 0.f;

      // define wedge parameters
      const float wSize = .4f;
      const float wX = .5f, wY = -.5f, wZ = 0.f;

      // define tetrahedron parameters
      const float tSize = .4f;
      const float tX = .5f, tY = .5f, tZ = 0.f;

      // define pyramid parameters
      const float pSize = .4f;
      const float pX = -.5f, pY = .5f, pZ = 0.f;

      // define vertex positions
      std::vector<vec3f> vertices =
      {
        // hexahedron
        { -hSize + hX, -hSize + hY,  hSize + hZ }, // bottom quad
        {  hSize + hX, -hSize + hY,  hSize + hZ },
        {  hSize + hX, -hSize + hY, -hSize + hZ },
        { -hSize + hX, -hSize + hY, -hSize + hZ },
        { -hSize + hX,  hSize + hY,  hSize + hZ }, // top quad
        {  hSize + hX,  hSize + hY,  hSize + hZ },
        {  hSize + hX,  hSize + hY, -hSize + hZ },
        { -hSize + hX,  hSize + hY, -hSize + hZ },

        // wedge
        { -wSize + wX, -wSize + wY,  wSize + wZ }, // botom triangle
        {  wSize + wX, -wSize + wY,    0.f + wZ },
        { -wSize + wX, -wSize + wY, -wSize + wZ },
        { -wSize + wX,  wSize + wY,  wSize + wZ }, // top triangle
        {  wSize + wX,  wSize + wY,    0.f + wZ },
        { -wSize + wX,  wSize + wY, -wSize + wZ },

        // tetrahedron
        { -tSize + tX, -tSize + tY,  tSize + tZ },
        {  tSize + tX, -tSize + tY,    0.f + tZ },
        { -tSize + tX, -tSize + tY, -tSize + tZ },
        { -tSize + tX,  tSize + tY,    0.f + tZ },

        // pyramid
        { -pSize + pX, -pSize + pY,  pSize + pZ },
        {  pSize + pX, -pSize + pY,  pSize + pZ },
        {  pSize + pX, -pSize + pY, -pSize + pZ },
        { -pSize + pX, -pSize + pY, -pSize + pZ },
        {  pSize + pX,  pSize + pY,  0.f + pZ }
      };

      // define per-vertex values
      std::vector<float> vertexValues =
      {
        // hexahedron
        0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,

        // wedge
        0.f, 0.f, 0.f, 1.f, 0.f, 1.f,

        // tetrahedron
        1.f, 0.f, 1.f, 0.f,

        // pyramid
        0.f, 1.f, 1.f, 0.f, 0.f
      };

      std::vector<uint32_t> indices =
      {
        // hexahedron
        0, 1, 2, 3, 4, 5, 6, 7,

#ifdef SHARED_VERTICES
        // wedge
        1, 9, 2, 5, 12, 6,

        // tetrahedron
        5, 12, 6, 17,

        // pyramid
        4, 5, 6, 7, 17
#else
        // wedge
        8, 9, 10, 11, 12, 13,

        // tetrahedron
        14, 15, 16, 17,

        // pyramid
        18, 19, 20, 21, 22
#endif
      };

      // define cell offsets in indices array
      std::vector<uint32_t> cells = { 0, 8, 14, 18 };

      // define cell types
      std::vector<uint8_t> cellTypes =
        { OSP_HEXAHEDRON, OSP_WEDGE, OSP_TETRAHEDRON, OSP_PYRAMID };

      // define per-cell values
      std::vector<float> cellValues = { 0.1f, .3f, .7f, 1.f };

      // create data objects
      OSPData verticesData = ospNewData(
        vertices.size(), OSP_FLOAT3, vertices.data());
      OSPData vertexValuesData = ospNewData(
        vertexValues.size(), OSP_FLOAT, vertexValues.data());
      OSPData indicesData = ospNewData(
        indices.size(), OSP_UINT, indices.data());
      OSPData cellsData = ospNewData(
        cells.size(), OSP_UINT, cells.data());
      OSPData cellTypesData = ospNewData(
        cellTypes.size(), OSP_UCHAR, cellTypes.data());
      OSPData cellValuesData = ospNewData(
        cellValues.size(), OSP_FLOAT, cellValues.data());

      // calculate bounds
      box3f bounds;
      std::for_each(vertices.begin(), vertices.end(), [&](vec3f &v) {
        bounds.extend(v);
      });

      // set data objects for volume object
      ospSetData(volume, "vertex", verticesData);
#if !defined(VALUES_PER_CELL)
      ospSetData(volume, "vertex.value", vertexValuesData);
#endif
      ospSetData(volume, "index", indicesData);
      ospSetData(volume, "cell", cellsData);
      ospSetData(volume, "cell.type", cellTypesData);
#ifdef VALUES_PER_CELL
      ospSetData(volume, "cell.value", cellValuesData);
#endif

      // release handlers that go out of scope here
      ospRelease(verticesData);
      ospRelease(vertexValuesData);
      ospRelease(indicesData);
      ospRelease(cellsData);
      ospRelease(cellTypesData);
      ospRelease(cellValuesData);

      // create OSPRay objects and return results
      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = osp_vec2f{0.f, 1.f};
      retval.bounds     = reinterpret_cast<const osp_box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(SimpleUnStructuredVolume, unstructured_volume);
    OSP_REGISTER_TESTING_VOLUME(SimpleUnStructuredVolume,
                                simple_unstructured_volume);

  }  // namespace testing
}  // namespace ospray
