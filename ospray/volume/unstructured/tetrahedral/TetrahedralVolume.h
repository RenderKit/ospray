// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "../../Volume.h"
#include "../../../common/OSPCommon.h"
#include "../MinMaxBVH2.h"

namespace ospray {

  class TetrahedralVolume : public Volume
  {
   public:

    struct TetFaceNormal
    {
      vec3f normals[4];
      vec3f corners[4];
    };

    ~TetrahedralVolume() = default;

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

    void getTetBBox(size_t id, box4f &bbox);

    //! Create the equivalent ISPC volume container.
    void createEquivalentISPC();

    //! Complete volume initialization (only on first commit).
    void finish() override;

    // Data members //

    int nVertices;
    std::vector<vec3f> vertices;

    int nTetrahedra;
    std::vector<vec4i> tetrahedra;

    std::vector<float> field;  // Attribute value at each vertex.

    box3f bbox;

    MinMaxBVH bvh;

    bool finished {false};

  };

}  // ::ospray
