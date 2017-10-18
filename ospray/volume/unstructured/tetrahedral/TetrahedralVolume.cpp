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

// ospray
#include "TetrahedralVolume.h"
#include "../../../common/Data.h"

// ospcommon
#include "ospcommon/tasking/parallel_for.h"

// auto-generated .h file.
#include "TetrahedralVolume_ispc.h"

namespace ospray {

  TetrahedralVolume::TetrahedralVolume()
  {
    ispcEquivalent = ispc::TetrahedralVolume_createInstance(this);
  }

  std::string TetrahedralVolume::toString() const
  {
    return std::string("ospray::TetrahedralVolume");
  }

  void TetrahedralVolume::commit()
  {
    updateEditableParameters();
    if (!finished)
      finish();
    Volume::commit();
  }

  int TetrahedralVolume::setRegion(const void *, const vec3i &, const vec3i &)
  {
    return 0;
  }

  void TetrahedralVolume::computeSamples(float **,
                                         const vec3f *,
                                         const size_t &)
  {
    NOT_IMPLEMENTED;
  }

  box4f TetrahedralVolume::getTetBBox(size_t id)
  {
    auto t = tetrahedra[id];

    box4f tetBox;

    for (int i = 0; i < 4; i++) {
      const auto &v = vertices[t[i]];
      const float f = field[t[i]];
      const auto p  = vec4f(v.x, v.y, v.z, f);

      if (i == 0)
        tetBox.upper = tetBox.lower = p;
      else
        tetBox.extend(p);
    }

    return tetBox;
  }

  void TetrahedralVolume::finish()
  {
    Data *verticesData   = getParamData("vertices", nullptr);
    Data *tetrahedraData = getParamData("tetrahedra", nullptr);
    Data *fieldData      = getParamData("field", nullptr);

    if (!verticesData || !tetrahedraData || !fieldData) {
      throw std::runtime_error(
          "#osp: missing correct data arrays in "
          " TetrahedralVolume!");
    }

    nVertices   = verticesData->size();
    nTetrahedra = tetrahedraData->size();

    vertices   = (vec3f *)verticesData->data;
    tetrahedra = (vec4i *)tetrahedraData->data;
    field      = (float *)fieldData->data;

    buildBvhAndCalculateBounds();
    calculateFaceNormals();

    float samplingRate = getParam1f("samplingRate", 1.f);
    float samplingStep = calculateSamplingStep();

    TetrahedralVolume_set(ispcEquivalent,
                          nVertices,
                          nTetrahedra,
                          (const ispc::box3f &)bbox,
                          (const ispc::vec3f *)vertices,
                          (const ispc::vec3f *)faceNormals.data(),
                          (const ispc::vec4i *)tetrahedra,
                          (const float *)field,
                          bvh.rootRef(),
                          bvh.nodePtr(),
                          bvh.itemListPtr(),
                          samplingRate,
                          samplingStep);

    Volume::finish();
    finished = true;
  }

  void TetrahedralVolume::buildBvhAndCalculateBounds()
  {
    std::vector<int64> primID(nTetrahedra);
    std::vector<box4f> primBounds(nTetrahedra);

    for (int i = 0; i < nTetrahedra; i++) {
      primID[i]   = i;
      auto bounds = getTetBBox(i);
      if (i == 0) {
        bbox.lower = vec3f(bounds.lower.x, bounds.lower.y, bounds.lower.z);
        bbox.upper = vec3f(bounds.upper.x, bounds.upper.y, bounds.upper.z);
      } else {
        bbox.extend(vec3f(bounds.lower.x, bounds.lower.y, bounds.lower.z));
        bbox.extend(vec3f(bounds.upper.x, bounds.upper.y, bounds.upper.z));
      }
      primBounds[i] = bounds;
    }

    bvh.build(primBounds.data(), primID.data(), nTetrahedra);
  }

  void TetrahedralVolume::calculateFaceNormals()
  {
    const auto numNormals = nTetrahedra * 4;
    faceNormals.resize(numNormals);

    tasking::parallel_for(numNormals / 4, [&](int taskIndex) {
      const int i   = taskIndex * 4;
      const auto &t = tetrahedra[i / 4];

      // The corners of each triangle in the tetrahedron.
      const int faces[4][3] = {{1, 2, 3}, {2, 0, 3}, {3, 0, 1}, {0, 2, 1}};

      for (int j = 0; j < 4; j++) {
        const int t0 = t[faces[j][0]];
        const int t1 = t[faces[j][1]];
        const int t2 = t[faces[j][2]];

        const auto &p0 = vertices[t0];
        const auto &p1 = vertices[t1];
        const auto &p2 = vertices[t2];

        const auto q0 = p1 - p0;
        const auto q1 = p2 - p0;

        const auto norm = normalize(cross(q0, q1));

        faceNormals[i + j] = norm;
      }
    });
  }

  float TetrahedralVolume::calculateSamplingStep()
  {
    float dx = bbox.upper.x - bbox.lower.x;
    float dy = bbox.upper.y - bbox.lower.y;
    float dz = bbox.upper.z - bbox.lower.z;

    float d            = std::min(std::min(dx, dy), dz);
    float samplingStep = d * 0.01f;

    return getParam1f("samplingStep", samplingStep);
  }

  OSP_REGISTER_VOLUME(TetrahedralVolume, tetrahedral_volume);

}  // ::ospray