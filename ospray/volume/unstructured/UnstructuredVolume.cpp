// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "UnstructuredVolume.h"
#include "../../common/Data.h"

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"

// auto-generated .h file.
#include "UnstructuredVolume_ispc.h"

namespace ospray {

  UnstructuredVolume::UnstructuredVolume()
  {
    ispcEquivalent = ispc::UnstructuredVolume_createInstance(this);
  }

  std::string UnstructuredVolume::toString() const
  {
    return std::string("ospray::UnstructuredVolume");
  }

  void UnstructuredVolume::commit()
  {
    updateEditableParameters();

    if (getParamData("field", nullptr) != oldField ||
        getParamData("cellField", nullptr) != oldCellField) {
      oldField = getParamData("field", nullptr);
      oldCellField = getParamData("cellField", nullptr);

      // rebuild BVH, resync ISPC, etc...
      finished = false;
    }

    if (!finished)
      finish();

    auto methodStringFromEnv =
      utility::getEnvVar<std::string>("OSPRAY_HEX_METHOD");
    std::string methodString =
      methodStringFromEnv.value_or(getParamString("hexMethod","planar"));
    if (methodString == "planar") {
      ispc::UnstructuredVolume_method_planar(ispcEquivalent);
    } else if (methodString == "nonplanar") {
      ispc::UnstructuredVolume_method_nonplanar(ispcEquivalent);
    }

    ispc::UnstructuredVolume_disableCellGradient(ispcEquivalent);

    if (getParam<int>("precomputedNormals", 1)) {
      if (faceNormals.empty()) {
        calculateFaceNormals();
        ispc::UnstructuredVolume_setFaceNormals(ispcEquivalent,
                                                (const ispc::vec3f *)faceNormals.data());
      }
    } else {
      if (!faceNormals.empty()) {
        ispc::UnstructuredVolume_setFaceNormals(ispcEquivalent,
                                                (const ispc::vec3f *)nullptr);
        faceNormals.clear();
        faceNormals.shrink_to_fit();
      }
    }

    Volume::commit();
  }

  int UnstructuredVolume::setRegion(const void *, const vec3i &, const vec3i &)
  {
    return 0;
  }

  void UnstructuredVolume::computeSamples(float **,
                                         const vec3f *,
                                         const size_t &)
  {
    NOT_IMPLEMENTED;
  }

  box4f UnstructuredVolume::getTetBBox(size_t id)
  {
    box4f tetBox;

    int maxIdx;

    switch (indices[2 * id][0]) {
    case -1:
      maxIdx = 4;
      break;
    case -2:
      maxIdx = 6;
      break;
    default:
      maxIdx = 8;
      break;
    }

    for (int i = 0; i < maxIdx; i++) {
      size_t idx = 0;
      switch (maxIdx) {
      case 4:
        idx = indices[2 * id + 1][i];
        break;
      case 6:
        if (i < 2)
          idx = indices[2 * id][1 + 2];
        else
          idx = indices[2 * id + 1][i - 2];
        break;
      case 8:
        if (i < 4)
          idx = indices[2 * id][i];
        else
          idx = indices[2 * id + 1][i - 4];
        break;
      }
      const auto &v = vertices[idx];
      const float f = cellField ? cellField[id] : field[idx];
      const auto p  = vec4f(v.x, v.y, v.z, f);

      if (i == 0)
        tetBox.upper = tetBox.lower = p;
      else
        tetBox.extend(p);
    }

    return tetBox;
  }

  void UnstructuredVolume::fixupTetWinding()
  {
    tasking::parallel_for(nCells, [&](int i) {
      if (indices[2 * i].x != -1)
        return;

      auto &idx = indices[2 * i + 1];
      const auto &p0 = vertices[idx.x];
      const auto &p1 = vertices[idx.y];
      const auto &p2 = vertices[idx.z];
      const auto &p3 = vertices[idx.w];

      auto center = (p0 + p1 + p2 + p3) / 4;
      auto norm = cross(p1 - p0, p2 - p0);
      auto dist = dot(norm, p0 - center);

      if (dist > 0.f)
        std::swap(idx.x, idx.y);
    });
  }

  void UnstructuredVolume::finish()
  {
    Data *verticesData   = getParamData("vertices", nullptr);
    Data *indicesData    = getParamData("indices", nullptr);
    Data *fieldData      = getParamData("field", nullptr);
    Data *cellFieldData  = getParamData("cellField", nullptr);

    if (!verticesData || !indicesData || (!fieldData && !cellFieldData)) {
      throw std::runtime_error(
          "#osp: missing correct data arrays in "
          " UnstructuredVolume!");
    }

    nVertices   = verticesData->size();

    nCells = indicesData->size() / 2;
    indices = (vec4i *)indicesData->data;

    vertices   = (vec3f *)verticesData->data;
    field      = fieldData ? (float *)fieldData->data : nullptr;
    cellField  = cellFieldData ? (float *)cellFieldData->data : nullptr;

    buildBvhAndCalculateBounds();
    fixupTetWinding();

    float samplingRate = getParam1f("samplingRate", 1.f);
    float samplingStep = calculateSamplingStep();

    UnstructuredVolume_set(ispcEquivalent,
                          nVertices,
                          nCells,
                          (const ispc::box3f &)bbox,
                          (const ispc::vec3f *)vertices,
                          (const ispc::vec4i *)indices,
                          (const float *)field,
                          (const float *)cellField,
                          bvh.rootRef(),
                          bvh.nodePtr(),
                          bvh.itemListPtr(),
                          samplingRate,
                          samplingStep);

    Volume::finish();
    finished = true;
  }

  void UnstructuredVolume::buildBvhAndCalculateBounds()
  {
    std::vector<int64> primID(nCells);
    std::vector<box4f> primBounds(nCells);

    for (int i = 0; i < nCells; i++) {
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

    bvh.build(primBounds.data(), primID.data(), nCells);
  }

  void UnstructuredVolume::calculateFaceNormals()
  {
    const auto numNormals = nCells * 6;
    faceNormals.resize(numNormals);

    tasking::parallel_for(numNormals / 6, [&](int taskIndex) {
      const int i   = taskIndex * 6;

      if (indices[2 * taskIndex].x == -1) {
        // tetrahedron cell
        const vec4i &t = indices[2 * taskIndex + 1];

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
      } else if (indices[2 * taskIndex].x == -2) {
        // wedge cell
        const vec4i &lower = indices[2 * taskIndex];
        const vec4i &upper = indices[2 * taskIndex + 1];

        const auto v0 = vertices[lower.z];
        const auto v1 = vertices[lower.w];
        const auto v2 = vertices[upper.x];
        const auto v3 = vertices[upper.y];
        const auto v4 = vertices[upper.z];
        const auto v5 = vertices[upper.w];

        faceNormals[i + 0] = normalize(cross(v2 - v0, v1 - v0)); // bottom
        faceNormals[i + 1] = normalize(cross(v4 - v3, v5 - v3)); // top
        faceNormals[i + 2] = normalize(cross(v3 - v0, v2 - v0));
        faceNormals[i + 3] = normalize(cross(v4 - v1, v0 - v1));
        faceNormals[i + 4] = normalize(cross(v5 - v2, v1 - v2));
      } else {
        // hexahedron cell
        const vec4i &lower = indices[2 * taskIndex];
        const vec4i &upper = indices[2 * taskIndex + 1];

        const auto v0 = vertices[lower.x];
        const auto v1 = vertices[lower.y];
        const auto v2 = vertices[lower.z];
        const auto v3 = vertices[lower.w];
        const auto v4 = vertices[upper.x];
        const auto v5 = vertices[upper.y];
        const auto v6 = vertices[upper.z];
        const auto v7 = vertices[upper.w];

        faceNormals[i + 0] = normalize(cross(v2 - v0, v1 - v0));
        faceNormals[i + 1] = normalize(cross(v5 - v0, v4 - v0));
        faceNormals[i + 2] = normalize(cross(v7 - v0, v3 - v0));
        faceNormals[i + 3] = normalize(cross(v5 - v6, v1 - v6));
        faceNormals[i + 4] = normalize(cross(v7 - v6, v4 - v6));
        faceNormals[i + 5] = normalize(cross(v2 - v6, v3 - v6));
      }
    });
  }

  float UnstructuredVolume::calculateSamplingStep()
  {
    float dx = bbox.upper.x - bbox.lower.x;
    float dy = bbox.upper.y - bbox.lower.y;
    float dz = bbox.upper.z - bbox.lower.z;

    float d            = std::min(std::min(dx, dy), dz);
    float samplingStep = d * 0.01f;

    return getParam1f("samplingStep", samplingStep);
  }

  OSP_REGISTER_VOLUME(UnstructuredVolume, unstructured_volume);

}  // ::ospray
