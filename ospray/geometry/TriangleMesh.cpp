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
#include "TriangleMesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "TriangleMesh_ispc.h"

namespace ospray {

  std::string TriangleMesh::toString() const
  {
    return "ospray::TriangleMesh";
  }

  void TriangleMesh::commit()
  {
    Geometry::commit();

    vertexData   = getParamData("vertex.position");
    normalData   = getParamData("vertex.normal");
    colorData    = getParamData("vertex.color");
    texcoordData = getParamData("vertex.texcoord");
    indexData    = getParamData("index");

    if (!vertexData)
      throw std::runtime_error("triangle mesh must have 'vertex' array");
    if (!indexData)
      throw std::runtime_error("triangle mesh must have 'index' array");
    if (colorData && colorData->type != OSP_VEC4F &&
        colorData->type != OSP_VEC3FA)
      throw std::runtime_error(
          "vertex.color must have data type OSP_VEC4F or OSP_VEC3FA");

    // check whether we need 64-bit addressing
    huge_mesh = false;
    if (indexData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (vertexData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (normalData && normalData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (colorData && colorData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (texcoordData && texcoordData->numBytes > INT32_MAX)
      huge_mesh = true;

    this->index    = (int *)indexData->data;
    this->vertex   = (float *)vertexData->data;
    this->normal   = normalData ? (float *)normalData->data : nullptr;
    this->color    = colorData ? (vec4f *)colorData->data : nullptr;
    this->texcoord = texcoordData ? (vec2f *)texcoordData->data : nullptr;

    numTris  = -1;
    numVerts = -1;

    numCompsInTri = 0;
    numCompsInVtx = 0;
    numCompsInNor = 0;

    switch (indexData->type) {
    case OSP_INT:
    case OSP_UINT:
      numTris       = indexData->size() / 3;
      numCompsInTri = 3;
      break;
    case OSP_VEC3I:
    case OSP_VEC3UI:
      numTris       = indexData->size();
      numCompsInTri = 3;
      break;
    case OSP_VEC4UI:
    case OSP_VEC4I:
      numTris       = indexData->size();
      numCompsInTri = 4;
      break;
    default:
      throw std::runtime_error("unsupported trianglemesh.index data type");
    }

    switch (vertexData->type) {
    case OSP_FLOAT:
      numVerts      = vertexData->size() / 4;
      numCompsInVtx = 4;
      break;
    case OSP_VEC3F:
      numVerts      = vertexData->size();
      numCompsInVtx = 3;
      break;
    case OSP_VEC3FA:
      numVerts      = vertexData->size();
      numCompsInVtx = 4;
      break;
    case OSP_VEC4F:
      numVerts      = vertexData->size();
      numCompsInVtx = 4;
      break;
    default:
      throw std::runtime_error("unsupported trianglemesh.vertex data type");
    }

    if (normalData) {
      switch (normalData->type) {
      case OSP_VEC3F:
        numCompsInNor = 3;
        break;
      case OSP_FLOAT:
      case OSP_VEC3FA:
        numCompsInNor = 4;
        break;
      default:
        throw std::runtime_error(
            "unsupported trianglemesh.vertex.normal data type");
      }
    }

    postStatusMsg(2) << "  created triangle mesh (" << numTris << " tris, "
                     << numVerts << " vertices)\n";
  }

  size_t TriangleMesh::numPrimitives() const
  {
    return indexData ? indexData->numItems / 3 : 0;
  }

  LiveGeometry TriangleMesh::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::TriangleMesh_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_TRIANGLE);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_INDEX,
                               0,
                               RTC_FORMAT_UINT3,
                               indexData->data,
                               0,
                               numCompsInTri * sizeof(int),
                               numTris);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_VERTEX,
                               0,
                               RTC_FORMAT_FLOAT3,
                               vertexData->data,
                               0,
                               numCompsInVtx * sizeof(int),
                               numVerts);

    rtcCommitGeometry(retval.embreeGeometry);

    ispc::TriangleMesh_set(retval.ispcEquivalent,
                           retval.embreeGeometry,
                           numTris,
                           numCompsInTri,
                           numCompsInVtx,
                           numCompsInNor,
                           (int *)index,
                           (float *)vertex,
                           (float *)normal,
                           (ispc::vec4f *)color,
                           (ispc::vec2f *)texcoord,
                           colorData && colorData->type == OSP_VEC4F,
                           huge_mesh);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(TriangleMesh, triangles);
  OSP_REGISTER_GEOMETRY(TriangleMesh, trianglemesh);

}  // namespace ospray
