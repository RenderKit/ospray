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

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"

// auto-generated .h file.
#include "UnstructuredVolume_ispc.h"

namespace ospray {

  namespace {
    // Map cell type to its vertices count
    inline uint32_t getVerticesCount(OSPUnstructuredCellType cellType)
    {
      switch (cellType) {
      case OSP_TETRAHEDRON:
        return 4;
      case OSP_HEXAHEDRON:
        return 8;
      case OSP_WEDGE:
        return 6;
      case OSP_PYRAMID:
        return 5;
      case OSP_UNKNOWN_CELL_TYPE:
        break;
      }

      return 1;
    }
  }  // namespace

  UnstructuredVolume::UnstructuredVolume()
  {
    ispcEquivalent = ispc::UnstructuredVolume_createInstance(this);
  }

  UnstructuredVolume::~UnstructuredVolume()
  {
    // free cells array memory if allocated
    if (cellArrayToDelete)
      delete[] cell;
    if (cellTypeArrayToDelete)
      delete[] cellType;
  };

  std::string UnstructuredVolume::toString() const
  {
    return "ospray::UnstructuredVolume";
  }

  void UnstructuredVolume::commit()
  {
    Volume::commit();

    // check if value buffer has changed
    if (getParamData("vertex.value") != vertexValuePrev ||
        getParamData("cell.value") != cellValuePrev) {
      vertexValuePrev = getParamData("vertex.value");
      cellValuePrev   = getParamData("cell.value");

      // rebuild BVH, resync ISPC, etc...
      finished = false;
    }

    OSPUnstructuredMethod method =
        (OSPUnstructuredMethod)getParam<int>("hexMethod", OSP_FAST);
    if (method == OSP_FAST)
      ispc::UnstructuredVolume_method_fast(ispcEquivalent);
    else if (method == OSP_ITERATIVE)
      ispc::UnstructuredVolume_method_iterative(ispcEquivalent);
    else {
      throw std::runtime_error(
          "unstructured volume 'hexMethod' has invalid type. Must be one of: "
          "OSP_FAST, OSP_ITERATIVE");
    }

    ispc::UnstructuredVolume_disableCellGradient(ispcEquivalent);

    if (getParam<bool>("precomputedNormals", false)) {
      if (faceNormals.empty()) {
        calculateFaceNormals();
        ispc::UnstructuredVolume_setFaceNormals(
            ispcEquivalent, (const ispc::vec3f *)faceNormals.data());
      }
    } else {
      if (!faceNormals.empty()) {
        ispc::UnstructuredVolume_setFaceNormals(ispcEquivalent,
                                                (const ispc::vec3f *)nullptr);
        faceNormals.clear();
        faceNormals.shrink_to_fit();
      }
    }

    if (finished)
      return;

    // read arrays given through API
    Data *vertexData        = getParamData("vertex.position");
    Data *vertexValueData   = getParamData("vertex.value");
    Data *indexData         = getParamData("index");
    Data *indexPrefixedData = getParamData("indexPrefixed");
    Data *cellData          = getParamData("cell");
    Data *cellValueData     = getParamData("cell.value");
    Data *cellTypeData      = getParamData("cell.type");

    // make sure that all necessary arrays are provided
    if (!vertexData) {
      throw std::runtime_error("unstructured volume must have 'vertex' array");
    }

    if (!indexData && !indexPrefixedData) {
      throw std::runtime_error(
          "unstructured volume must have 'index' or 'indexPrefixed' array");
    }

    if (!vertexValueData && !cellValueData) {
      throw std::runtime_error(
          "unstructured volume must have 'vertex.value' or 'cell.value' array");
    }

    // if index array is prefixed with cell size
    if (indexPrefixedData) {
      indexData   = indexPrefixedData;
      cellSkipIds = 1;  // skip one index per cell, it will contain cell size
    } else {
      cellSkipIds = 0;
    }

    // retrieve array pointers
    vertex = (vec3f *)vertexData->data();
    index = (uint32_t *)indexData->data();
    vertexValue = vertexValueData ? (float *)vertexValueData->data() : nullptr;
    cellValue = cellValueData ? (float *)cellValueData->data() : nullptr;

    // set index integer size based on index data type
    switch (indexData->type) {
    case OSP_INT:
    case OSP_UINT:
    case OSP_VEC4I:
    case OSP_VEC4UI:
      index32Bit = true;
      break;
    case OSP_LONG:
    case OSP_ULONG:
      index32Bit = false;
      break;
    default: {
      throw std::runtime_error(
          "unstructured volume 'index' array has invalid type " +
          stringFor(indexData->type) +
          ". Must be one of: OSP_INT, OSP_UINT, OSP_VEC4I, OSP_VEC4UI, "
          "OSP_LONG, OSP_ULONG");
    }
    }

    // 'cell' parameter is optional
    if (cellData) {
      // intialize cells with data given through API
      nCells = cellData->size();
      cell = (uint32_t *)cellData->data();

      // set cell integer size based on cell data type
      switch (cellData->type) {
      case OSP_INT:
      case OSP_UINT:
      case OSP_VEC4I:
      case OSP_VEC4UI:
        cell32Bit = true;
        break;
      case OSP_LONG:
      case OSP_ULONG:
        cell32Bit = false;
        break;
      default: {
        throw std::runtime_error(
            "unstructured volume 'cell' array has invalid type " +
            stringFor(cellData->type) +
            ". Must be one of: OSP_INT, OSP_UINT, OSP_VEC4I, OSP_VEC4UI, "
            "OSP_LONG, OSP_ULONG");
      }
      }
    } else {
      // if cells array was not provided through API allocate it
      // and fill with default cell offsets
      nCells = (indexData->type == OSP_VEC4UI) || (indexData->type == OSP_VEC4I)
                   ? indexData->size() / 2
                   : indexData->size() / 8;
      cell              = new uint32_t[nCells];
      cell32Bit         = true;
      cellArrayToDelete = true;

      // calculate cell offsets assuming legacy layout:
      // tetrahedron: -1, -1, -1, -1, i1, i2, i3, i4
      // wedge: -2, -2, i1, i2, i3, i4, i5, i6
      // hexahedron: i1, i2, i3, i4, i5, i6, i7, i8
      for (uint64_t i = 0; i < nCells; i++)
        switch (index[8 * i]) {
        case -1:
          cell[i] = 8 * i + 4;
          break;
        case -2:
          cell[i] = 8 * i + 2;
          break;
        default:
          cell[i] = 8 * i;
          break;
        }
    }

    // 'cell.type' parameter is optional
    if (cellTypeData) {
      cellType = (OSPUnstructuredCellType *)cellTypeData->data();

      // check if number of cell types matches number of cells
      if (cellTypeData->size() != nCells) {
        throw std::runtime_error(
            "unstructured volume 'cell.type' array length does not match "
            "number of cells");
      }

    } else {
      // create cell type array
      cellType              = new OSPUnstructuredCellType[nCells];
      cellTypeArrayToDelete = true;

      // map type values from indices array
      for (uint64_t i = 0; i < nCells; i++) {
        switch (index[8 * i]) {
        case -1:
          cellType[i] = OSP_TETRAHEDRON;
          break;
        case -2:
          cellType[i] = OSP_WEDGE;
          break;
        default:
          cellType[i] = OSP_HEXAHEDRON;
          break;
        }
      }
    }

    buildBvhAndCalculateBounds();

    float samplingStep = calculateSamplingStep();

    UnstructuredVolume_set(ispcEquivalent,
                           (const ispc::box3f &)bounds,
                           (const ispc::vec3f *)vertex,
                           index,
                           index32Bit,
                           vertexValue,
                           cellValue,
                           cell,
                           cell32Bit,
                           cellSkipIds,
                           (uint8_t *)cellType,
                           bvh.rootRef(),
                           bvh.nodePtr(),
                           bvh.itemListPtr(),
                           samplingStep);

    finished = true;
  }

  box4f UnstructuredVolume::getCellBBox(size_t id)
  {
    // get cell offset in the vertex indices array
    uint64_t cOffset = getCellOffset(id);

    // iterate through cell vertices
    box4f bBox;
    uint32_t maxIdx = getVerticesCount(cellType[id]);
    for (uint32_t i = 0; i < maxIdx; i++) {
      // get vertex index
      uint64_t vId = getVertexId(cOffset + i);

      // build 4 dimensional vertex with its position and value
      vec3f &v  = vertex[vId];
      float val = cellValue ? cellValue[id] : vertexValue[vId];
      vec4f p   = vec4f(v.x, v.y, v.z, val);

      // extend bounding box
      if (i == 0)
        bBox.upper = bBox.lower = p;
      else
        bBox.extend(p);
    }

    return bBox;
  }

  void UnstructuredVolume::buildBvhAndCalculateBounds()
  {
    std::vector<int64> primID(nCells);
    std::vector<box4f> primBounds(nCells);

    for (uint64_t i = 0; i < nCells; i++) {
      primID[i]       = i;
      auto cellBounds = getCellBBox(i);
      if (i == 0) {
        bounds.lower =
            vec3f(cellBounds.lower.x, cellBounds.lower.y, cellBounds.lower.z);
        bounds.upper =
            vec3f(cellBounds.upper.x, cellBounds.upper.y, cellBounds.upper.z);
      } else {
        bounds.extend(
            vec3f(cellBounds.lower.x, cellBounds.lower.y, cellBounds.lower.z));
        bounds.extend(
            vec3f(cellBounds.upper.x, cellBounds.upper.y, cellBounds.upper.z));
      }
      primBounds[i] = cellBounds;
    }

    bvh.build(primBounds.data(), primID.data(), nCells);
  }

  void UnstructuredVolume::calculateFaceNormals()
  {
    // Allocate memory for normal vectors
    uint64_t numNormals = nCells * 6;
    faceNormals.resize(numNormals);

    // Define vertices order for normal calculation
    const uint32_t tetrahedronFaces[4][3] = {
        {2, 0, 1}, {3, 1, 0}, {3, 2, 1}, {2, 3, 0}};
    const uint32_t hexahedronFaces[6][3] = {
        {3, 0, 1}, {5, 1, 0}, {6, 2, 1}, {7, 3, 2}, {7, 4, 0}, {6, 5, 4}};
    const uint32_t wedgeFaces[5][3] = {
        {2, 0, 1}, {4, 1, 0}, {5, 2, 1}, {5, 3, 0}, {5, 4, 3}};
    const uint32_t pyramidFaces[5][3] = {
        {3, 0, 1}, {4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {3, 4, 0}};

    // Build all normals
    tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
      switch (cellType[taskIndex]) {
      case OSP_TETRAHEDRON:
        calculateCellNormals(taskIndex, tetrahedronFaces, 4);
        break;
      case OSP_HEXAHEDRON:
        calculateCellNormals(taskIndex, hexahedronFaces, 6);
        break;
      case OSP_WEDGE:
        calculateCellNormals(taskIndex, wedgeFaces, 5);
        break;
      case OSP_PYRAMID:
        calculateCellNormals(taskIndex, pyramidFaces, 5);
        break;
      case OSP_UNKNOWN_CELL_TYPE: {
        throw std::runtime_error(
            "unstructured volume has invalid cell type. Must be one of: "
            "OSP_TETRAHEDRON, OSP_HEXAHEDRON, OSP_WEDGE, OSP_PYRAMID");
      }
      }
    });
  }

  float UnstructuredVolume::calculateSamplingStep()
  {
    float dx = bounds.upper.x - bounds.lower.x;
    float dy = bounds.upper.y - bounds.lower.y;
    float dz = bounds.upper.z - bounds.lower.z;

    float d            = std::min(std::min(dx, dy), dz);
    float samplingStep = d * 0.01f;

    return getParam<float>("samplingStep", samplingStep);
  }

  OSP_REGISTER_VOLUME(UnstructuredVolume, unstructured_volume);

}  // namespace ospray
