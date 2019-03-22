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

namespace ospray {

  namespace {
    // Map cell type to its vertices count
    inline uint32_t getVerticesCount(uint8_t cellType)
    {
      switch (cellType) {
        case OSP_TETRAHEDRON: return 4;
        case OSP_HEXAHEDRON: return 8;
        case OSP_WEDGE: return 6;
        case OSP_PYRAMID: return 5;
      }

      // Unknown cell type
      return 1;
    }
  }

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
    uint32_t maxIdx = getVerticesCount(cellType[id]);
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
    // Allocate memory for normal vectors
    uint64_t numNormals = nCells * 6;
    faceNormals.resize(numNormals);

    // Define vertices order for normal calculation
    const uint32_t tetrahedronFaces[4][3] =
      {{1, 0, 2}, {0, 1, 3}, {1, 2, 3}, {0, 3, 2}};
    const uint32_t hexahedronFaces[6][3] =
      {{1, 0, 3}, {0, 1, 5}, {1, 2, 6}, {2, 3, 7}, {0, 4, 7}, {4, 5, 6}};
    const uint32_t wedgeFaces[5][3] =
      {{1, 0, 2}, {0, 1, 4}, {1, 2, 5}, {0, 3, 5}, {3, 4, 5}};
    const uint32_t pyramidFaces[5][3] =
      {{1, 0, 3}, {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {0, 4, 3}};

    // Build all normals
    tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
      switch(cellType[taskIndex]) {
        case OSP_TETRAHEDRON:
          calculateCellNormals(taskIndex, tetrahedronFaces, 4); break;
        case OSP_HEXAHEDRON:
          calculateCellNormals(taskIndex, hexahedronFaces, 6); break;
        case OSP_WEDGE:
          calculateCellNormals(taskIndex, wedgeFaces, 5); break;
        case OSP_PYRAMID:
          calculateCellNormals(taskIndex, pyramidFaces, 5); break;
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
