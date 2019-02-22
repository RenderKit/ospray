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

// ospray
#include "UnstructuredVolume.h"
#include "../../common/Data.h"
#include "ospray/OSPUnstructured.h"

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"

// auto-generated .h file.
#include "UnstructuredVolume_ispc.h"

namespace {
  // Map cell type to its vertices count
  inline uint32_t getCellIdCount(uint8_t cellType)
  {
    switch (cellType) {
      case OSP_TETRAHEDRON: return 4;
      case OSP_WEDGE: return 6;
      case OSP_HEXAHEDRON: return 8;
    }

    // Unknown cell type
    return 1;
  }
}

namespace ospray {

  UnstructuredVolume::UnstructuredVolume()
  {
    ispcEquivalent = ispc::UnstructuredVolume_createInstance(this);
  }

  UnstructuredVolume::~UnstructuredVolume()
  {
    // free cells array memory if allocated
    if (cellArrayToDelete)
      delete [] cell;
    if (cellTypeArrayToDelete)
      delete [] cellType;
  };

  std::string UnstructuredVolume::toString() const
  {
    return std::string("ospray::UnstructuredVolume");
  }

  void UnstructuredVolume::commit()
  {
    updateEditableParameters();

    // check if value buffer has changed
    if (getVertexValueData() != vertexValuePrev ||
      getCellValueData() != cellValuePrev) {
      vertexValuePrev = getVertexValueData();
      cellValuePrev = getCellValueData();

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

    if (getParam<bool>("precomputedNormals", true)) {
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

  box4f UnstructuredVolume::getCellBBox(size_t id)
  {
    // get cell offset in the vertex indices array
    uint64_t cOffset = getCellOffset(id);

    // iterate through cell vertices
    box4f bBox;
    uint32_t maxIdx = getCellIdCount(cellType[id]);
    for (uint32_t i = 0; i < maxIdx; i++)
    {
      // get vertex index
      uint64_t vId = getVertexId(cOffset + i);

      // build 4 dimensional vertex with its position and value
      vec3f& v = vertex[vId];
      float val = cellValue ? cellValue[id] : vertexValue[vId];
      vec4f p = vec4f(v.x, v.y, v.z, val);

      // extend bounding box
      if (i == 0)
        bBox.upper = bBox.lower = p;
      else
        bBox.extend(p);
    }

    return bBox;
  }

  void UnstructuredVolume::finish()
  {
    // read arrays given through API
    Data* vertexData = getParamData("vertex", getParamData("vertices"));
    Data* vertexValueData = getVertexValueData();
    Data* indexData = getParamData("index", getParamData("indices"));
    Data* indexPrefixedData = getParamData("indexPrefixed");
    Data* cellData = getParamData("cell");
    Data* cellValueData = getCellValueData();
    Data* cellTypeData = getParamData("cell.type");

    // make sure that all necessary arrays are provided
    if (!vertexData)
      throw std::runtime_error("unstructured volume must have 'vertex' array");
    if (!indexData && !indexPrefixedData)
      throw std::runtime_error(
        "unstructured volume must have 'index' or 'indexPrefixed' array");
    if (!vertexValueData && !cellValueData)
      throw std::runtime_error(
        "unstructured volume must have 'vertex.value' or 'cell.value' array");

    // if index array is prefixed with cell size
    if (indexPrefixedData) {
      indexData = indexPrefixedData;
      cellSkipIds = 1; // skip one index per cell, it will contain cell size
    } else {
      cellSkipIds = 0;
    }

    // retrieve array pointers
    vertex = (vec3f*)vertexData->data;
    index = (uint32_t*)indexData->data;
    vertexValue = vertexValueData ? (float*)vertexValueData->data : nullptr;
    cellValue = cellValueData ? (float*)cellValueData->data : nullptr;

    // set index integer size based on index data type
    switch (indexData->type) {
    case OSP_INT: case OSP_UINT: case OSP_UINT4: case OSP_INT4:
      index32Bit = true; break;
    case OSP_LONG: case OSP_ULONG:
      index32Bit = false; break;
    default:
      throw std::runtime_error("unsupported unstructured volume index data type");
    }

    // 'cell' parameter is optional
    if (cellData) {
      // intialize cells with data given through API
      nCells = cellData->size();
      cell = (uint32_t*)cellData->data;

      // set cell integer size based on cell data type
      switch (cellData->type) {
      case OSP_INT: case OSP_UINT: case OSP_UINT4: case OSP_INT4:
        cell32Bit = true; break;
      case OSP_LONG: case OSP_ULONG:
        cell32Bit = false; break;
      default:
        throw std::runtime_error("unsupported unstructured volume cell array data type");
      }
    } else {
      // if cells array was not provided through API allocate it
      // and fill with default cell offsets
      nCells =
        (indexData->type == OSP_UINT4) || (indexData->type == OSP_INT4) ?
        indexData->size() / 2 : indexData->size() / 8;
      cell = new uint32_t[nCells];
      cell32Bit = true;
      cellArrayToDelete = true;

      // calculate cell offsets assuming legacy layout:
      // tetrahedron: -1, -1, -1, -1, i1, i2, i3, i4
      // wedge: -2, -2, i1, i2, i3, i4, i5, i6
      // hexahedron: i1, i2, i3, i4, i5, i6, i7, i8
      for (uint64_t i = 0; i < nCells; i++)
        switch (index[8 * i]) {
        case -1: cell[i] = 8 * i + 4; break;
        case -2: cell[i] = 8 * i + 2; break;
        default: cell[i] = 8 * i; break;
        }
    }

    // 'cell.type' parameter is optional
    if (cellTypeData) {
      cellType = (uint8_t*)cellTypeData->data;

      // check if number of cell types matches number of cells
      if (cellTypeData->size() != nCells)
        throw std::runtime_error(
          "cell type array for unstructured volume has wrong size");
    } else {
      // create cell type array
      cellType = new uint8_t[nCells];
      cellTypeArrayToDelete = true;

      // map type values from indices array
      for (uint64_t i = 0; i < nCells; i++)
        switch (index[8 * i]) {
        case -1: cellType[i] = OSP_TETRAHEDRON; break;
        case -2: cellType[i] = OSP_WEDGE; break;
        default: cellType[i] = OSP_HEXAHEDRON; break;
        }
    }

    buildBvhAndCalculateBounds();

    float samplingRate = getParam1f("samplingRate", 1.f);
    float samplingStep = calculateSamplingStep();

    UnstructuredVolume_set(ispcEquivalent,
                          (const ispc::box3f &)bbox,
                          (const ispc::vec3f *)vertex,
                          index,
                          index32Bit,
                          vertexValue,
                          cellValue,
                          cell,
                          cell32Bit,
                          cellSkipIds,
                          cellType,
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

    for (uint64_t i = 0; i < nCells; i++) {
      primID[i]   = i;
      auto bounds = getCellBBox(i);
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
    uint64_t numNormals = nCells * 6;
    faceNormals.resize(numNormals);

    tasking::parallel_for(numNormals / 6, [&](uint64_t taskIndex) {

      // get cell offset in the vertex indices array
      uint64_t cOffset = getCellOffset(taskIndex);

      uint64_t i = taskIndex * 6;
      if (cellType[taskIndex] == OSP_TETRAHEDRON) {

        // The corners of each triangle in the tetrahedron.
        const int faces[4][3] = {{1, 2, 3}, {2, 0, 3}, {3, 0, 1}, {0, 2, 1}};

        for (int j = 0; j < 4; j++) {
          uint64_t vId0 = getVertexId(cOffset + faces[j][0]);
          uint64_t vId1 = getVertexId(cOffset + faces[j][1]);
          uint64_t vId2 = getVertexId(cOffset + faces[j][2]);

          const vec3f& p0 = vertex[vId0];
          const vec3f& p1 = vertex[vId1];
          const vec3f& p2 = vertex[vId2];

          vec3f q0 = p1 - p0;
          vec3f q1 = p2 - p0;

          vec3f norm = normalize(cross(q0, q1));

          faceNormals[i + j] = norm;
        }
      } else if (cellType[taskIndex] == OSP_WEDGE) {

        vec3f& v0 = vertex[getVertexId(cOffset + 0)];
        vec3f& v1 = vertex[getVertexId(cOffset + 1)];
        vec3f& v2 = vertex[getVertexId(cOffset + 2)];
        vec3f& v3 = vertex[getVertexId(cOffset + 3)];
        vec3f& v4 = vertex[getVertexId(cOffset + 4)];
        vec3f& v5 = vertex[getVertexId(cOffset + 5)];

        faceNormals[i + 0] = normalize(cross(v2 - v0, v1 - v0)); // bottom
        faceNormals[i + 1] = normalize(cross(v4 - v3, v5 - v3)); // top
        faceNormals[i + 2] = normalize(cross(v3 - v0, v2 - v0));
        faceNormals[i + 3] = normalize(cross(v4 - v1, v0 - v1));
        faceNormals[i + 4] = normalize(cross(v5 - v2, v1 - v2));

      } else if (cellType[taskIndex] == OSP_HEXAHEDRON) {

        vec3f& v0 = vertex[getVertexId(cOffset + 0)];
        vec3f& v1 = vertex[getVertexId(cOffset + 1)];
        vec3f& v2 = vertex[getVertexId(cOffset + 2)];
        vec3f& v3 = vertex[getVertexId(cOffset + 3)];
        vec3f& v4 = vertex[getVertexId(cOffset + 4)];
        vec3f& v5 = vertex[getVertexId(cOffset + 5)];
        vec3f& v6 = vertex[getVertexId(cOffset + 6)];
        vec3f& v7 = vertex[getVertexId(cOffset + 7)];

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
