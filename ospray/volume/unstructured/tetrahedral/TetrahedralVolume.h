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

    std::vector<box3f> tetBBoxes;

    int nVertices;
    std::vector<vec3f> vertices;

    int nTetrahedra;
    std::vector<vec4i> tetrahedra;

    std::vector<float> field;  // Attribute value at each vertex.

    box3f bbox;

    std::vector<TetFaceNormal> faceNormals;

    MinMaxBVH bvh;

    bool finished {false};

    void addTetBBox(int id)
    {
      auto &t    = tetrahedra[id];
      auto &bbox = tetBBoxes[id];
      for (int i = 0; i < 4; i++) {
        const auto &p = vertices[t[i]];
        if (i == 0) {
          bbox.lower.x = bbox.upper.x = p.x;
          bbox.lower.y = bbox.upper.y = p.y;
          bbox.lower.z = bbox.upper.z = p.z;
        } else {
          if (p.x < bbox.lower.x)
            bbox.lower.x = p.x;
          if (p.x > bbox.upper.x)
            bbox.upper.x = p.x;

          if (p.y < bbox.lower.y)
            bbox.lower.y = p.y;
          if (p.y > bbox.upper.y)
            bbox.upper.y = p.y;

          if (p.z < bbox.lower.z)
            bbox.lower.z = p.z;
          if (p.z > bbox.upper.z)
            bbox.upper.z = p.z;
        }
      }
    }

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

    float sample(float world_x, float world_y, float world_z);

    void getTetBBox(size_t id, box4f &bbox);

   protected:

    //! Create the equivalent ISPC volume container.
    void createEquivalentISPC();

    //! Complete volume initialization (only on first commit).
    void finish() override;
  };

}  // ::ospray
