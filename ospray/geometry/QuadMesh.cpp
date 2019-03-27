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
#include "QuadMesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "QuadMesh_ispc.h"

namespace ospray {

  QuadMesh::QuadMesh()
  {
    this->ispcEquivalent = ispc::QuadMesh_create(this);
  }

  std::string QuadMesh::toString() const
  {
    return "ospray::QuadMesh";
  }

  void QuadMesh::commit()
  {
    Geometry::commit();

    vertexData   = getParamData("vertex");
    normalData   = getParamData("vertex.normal", getParamData("normal"));
    colorData    = getParamData("vertex.color");
    texcoordData = getParamData("vertex.texcoord");
    indexData    = getParamData("index");

    if (!vertexData)
      throw std::runtime_error("quad mesh must have 'vertex' array");
    if (!indexData)
      throw std::runtime_error("quad mesh must have 'index' array");
    if (colorData && colorData->type != OSP_FLOAT4 &&
        colorData->type != OSP_FLOAT3A)
      throw std::runtime_error(
          "vertex.color must have data type OSP_FLOAT4 or OSP_FLOAT3A");

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

    switch (indexData->type) {
    case OSP_INT:
    case OSP_UINT:
      numQuads = indexData->size() / 4;
      break;
    case OSP_UINT4:
    case OSP_INT4:
      numQuads = indexData->size();
      break;
    default:
      throw std::runtime_error("unsupported quadmesh.index data type");
    }

    switch (vertexData->type) {
    case OSP_FLOAT:
      numVerts      = vertexData->size() / 4;
      numCompsInVtx = 4;
      break;
    case OSP_FLOAT3:
      numVerts      = vertexData->size();
      numCompsInVtx = 3;
      break;
    case OSP_FLOAT3A:
      numVerts      = vertexData->size();
      numCompsInVtx = 4;
      break;
    case OSP_FLOAT4:
      numVerts      = vertexData->size();
      numCompsInVtx = 4;
      break;
    default:
      throw std::runtime_error("unsupported quadmesh.vertex data type");
    }

    if (normalData) {
      switch (normalData->type) {
      case OSP_FLOAT3:
        numCompsInNor = 3;
        break;
      case OSP_FLOAT:
      case OSP_FLOAT3A:
        numCompsInNor = 4;
        break;
      default:
        throw std::runtime_error(
            "unsupported quadmesh.vertex.normal data type");
      }
    }

    bounds = empty;
    for (uint32_t i = 0; i < numVerts * numCompsInVtx; i += numCompsInVtx)
      bounds.extend(*(vec3f *)(vertex + i));

    createEmbreeGeometry();

    postStatusMsg(2) << "  created quad mesh (" << numQuads << " quads "
                     << ", " << numVerts << " vertices)\n"
                     << "  mesh bounds " << bounds;

    ispc::QuadMesh_set(getIE(),
                       embreeGeometry,
                       geomID,
                       numQuads,
                       numCompsInVtx,
                       numCompsInNor,
                       (ispc::vec4i *)index,
                       (float *)vertex,
                       (float *)normal,
                       (ispc::vec4f *)color,
                       (ispc::vec2f *)texcoord,
                       colorData && colorData->type == OSP_FLOAT4,
                       huge_mesh);
  }

  size_t QuadMesh::numPrimitives() const
  {
    return indexData ? indexData->numItems / 4 : 0;
  }

  void QuadMesh::createEmbreeGeometry()
  {
    if (embreeGeometry)
      rtcReleaseGeometry(embreeGeometry);

    embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_QUAD);

    rtcSetSharedGeometryBuffer(embreeGeometry,
                               RTC_BUFFER_TYPE_INDEX,
                               0,
                               RTC_FORMAT_UINT4,
                               indexData->data,
                               0,
                               4 * sizeof(int),
                               numQuads);

    rtcSetSharedGeometryBuffer(embreeGeometry,
                               RTC_BUFFER_TYPE_VERTEX,
                               0,
                               RTC_FORMAT_FLOAT3,
                               vertexData->data,
                               0,
                               numCompsInVtx * sizeof(int),
                               numVerts);

    rtcCommitGeometry(embreeGeometry);
  }

  OSP_REGISTER_GEOMETRY(QuadMesh, quads);
  OSP_REGISTER_GEOMETRY(QuadMesh, quadmesh);

}  // namespace ospray
