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
    ~UnstructuredVolume() override = default;

    //! A string description of this class.
    std::string toString() const override;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    void commit() override;

    //! Copy voxels into the volume at the given index
    /*! \returns 0 on error, any non-zero value indicates success */
    int setRegion(const void *source_pointer,
                  const vec3i &target_index,
                  const vec3i &source_count) override;

    //! Compute samples at the given world coordinates.
    void computeSamples(float **results,
                        const vec3f *worldCoordinates,
                        const size_t &count) override;

   private:
    box4f getTetBBox(size_t id);

    //! Complete volume initialization (only on first commit).
    void finish() override;

    void buildBvhAndCalculateBounds();
    void fixupTetWinding();
    void calculateFaceNormals();
    float calculateSamplingStep();

    // Data members //

    int nVertices;
    vec3f *vertices{nullptr};
    float *field{nullptr};  // Attribute value at each vertex.
    float *cellField{nullptr};  // Attribute value at each cell.

    Data *oldField{nullptr};
    Data *oldCellField{nullptr};

    int nCells;
    vec4i *indices{nullptr};

    std::vector<vec3f> faceNormals;

    box3f bbox;

    MinMaxBVH2 bvh;

    bool finished{false};
  };

}  // ::ospray
