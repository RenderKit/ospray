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

#pragma once

// ospray
#include "../../common/OSPCommon.h"
#include "../Volume.h"
#include "MinMaxBVH2.h"

namespace ospray {

  class UnstructuredVolume : public Volume
  {
   public:
    UnstructuredVolume();
    virtual ~UnstructuredVolume() override;

    //! A string description of this class.
    std::string toString() const override;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    void commit() override;

    //! Copy voxels into the volume at the given index
    /*! \returns 0 on error, any non-zero value indicates success */
    int setRegion(const void *source_pointer,
                  const vec3i &target_index,
                  const vec3i &source_count) override;

   private:

    // Helper functions for getting data array parameter
    inline Data* getCellValueData()
      { return getParamData("cell.value", getParamData("cellField")); }
    inline Data* getVertexValueData()
      { return getParamData("vertex.value", getParamData("field")); }

    // Read 32/64-bit integer value from given array
    inline uint64_t readInteger(const uint32_t* array,
                                bool is32Bit,
                                uint64_t id) const
    {
      if (!is32Bit) id <<= 1;
      uint64_t value = *((const uint64_t*)(array + id));
      if (is32Bit) value &= 0x00000000ffffffffull;
      return value;
    }

    // Read from index arrays that could have 32/64-bit element size
    inline uint64_t getCellOffset(uint64_t id) const
      { return readInteger(cell, cell32Bit, id) + cellSkipIds; }
    inline uint64_t getVertexId(uint64_t id) const
      { return readInteger(index, index32Bit, id); }

    // Calculate all normals for arbitrary polyhedron
    // based on given vertices order
    inline void calculateCellNormals(
      const uint64_t cellId,
      const uint32_t faces[6][3],
      const uint32_t facesCount)
    {
      // Get cell offset in the vertex indices array
      uint64_t cOffset = getCellOffset(cellId);

      // Iterate through all faces
      for (uint32_t i = 0; i < facesCount; i++) {

        // Retrieve vertex positions
        uint64_t vId0 = getVertexId(cOffset + faces[i][0]);
        uint64_t vId1 = getVertexId(cOffset + faces[i][1]);
        uint64_t vId2 = getVertexId(cOffset + faces[i][2]);
        const vec3f& v0 = vertex[vId0];
        const vec3f& v1 = vertex[vId1];
        const vec3f& v2 = vertex[vId2];

        // Calculate normal
        faceNormals[cellId * 6 + i] = normalize(cross(v0 - v1, v2 - v1));
      }
    }

    box4f getCellBBox(size_t id);

    void buildBvhAndCalculateBounds();
    void calculateFaceNormals();
    float calculateSamplingStep();

    // Vertex position array
    vec3f* vertex{nullptr};

    // Array of values defined per-vertex
    float* vertexValue{nullptr};
    Data* vertexValuePrev{nullptr}; // previous pointer to detect change

    // Array of cell offsets in the index array
    uint64_t nCells;
    uint32_t* cell{nullptr};
    bool cell32Bit{false};
    uint32_t cellSkipIds{0};       // skip indices when index array contain other data e.g. size
    bool cellArrayToDelete{false}; // to detect that deallocation is needed

    // Array of vertex indices
    uint32_t* index{nullptr};
    bool index32Bit{false};

    // Array of cell types
    uint8_t* cellType{nullptr};
    bool cellTypeArrayToDelete{false}; // to detect that deallocation is needed

    // Array of values defined per-cell
    float* cellValue{nullptr};
    Data* cellValuePrev{nullptr};

    std::vector<vec3f> faceNormals;

    MinMaxBVH2 bvh;

    bool finished{false};
  };

}  // ::ospray
