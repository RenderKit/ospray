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
#include "QuadMesh.h"
#include "common/Model.h"
#include "../include/ospray/ospray.h"
// ispc exports
#include "QuadMesh_ispc.h"
#include <cmath>

namespace ospray {

  inline bool inRange(int64 i, int64 i0, int64 i1)
  {
    return i >= i0 && i < i1;
  }

  QuadMesh::QuadMesh()
  {
    this->ispcEquivalent = ispc::QuadMesh_create(this);
  }

  std::string QuadMesh::toString() const
  {
    return "ospray::QuadMesh";
  }

  void QuadMesh::finalize(Model *model)
  {
    static int numPrints = 0;
    numPrints++;
    if (numPrints == 5) {
      postStatusMsg(2) << "(all future printouts for quad mesh creation "
                       << "will be omitted)";
    }

    if (numPrints < 5)
      postStatusMsg(2) << "ospray: finalizing quadmesh ...";

    Assert(model && "invalid model pointer");

    Geometry::finalize(model);

    RTCScene embreeSceneHandle = model->embreeSceneHandle;

    vertexData = getParamData("vertex");
    normalData = getParamData("vertex.normal",getParamData("normal"));
    colorData  = getParamData("vertex.color");
    texcoordData = getParamData("vertex.texcoord");
    indexData  = getParamData("index");
    prim_materialIDData = getParamData("prim.materialID");
    geom_materialID = getParam1i("geom.materialID",-1);

    if (!vertexData)
      throw std::runtime_error("quad mesh must have 'vertex' array");
    if (!indexData)
      throw std::runtime_error("quad mesh must have 'index' array");
    if (colorData && colorData->type != OSP_FLOAT4 && colorData->type != OSP_FLOAT3A)
      throw std::runtime_error("vertex.color must have data type OSP_FLOAT4 or OSP_FLOAT3A");

    // check whether we need 64-bit addressing
    bool huge_mesh = false;
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

    this->index = (int*)indexData->data;
    this->vertex = (float*)vertexData->data;
    this->normal = normalData ? (float*)normalData->data : nullptr;
    this->color  = colorData ? (vec4f*)colorData->data : nullptr;
    this->texcoord = texcoordData ? (vec2f*)texcoordData->data : nullptr;
    this->prim_materialID  = prim_materialIDData ? (uint32_t*)prim_materialIDData->data : nullptr;

    size_t numQuads  = -1;
    size_t numVerts = -1;

    size_t numCompsInVtx = 0;
    size_t numCompsInNor = 0;
    switch (indexData->type) {
    case OSP_INT:
    case OSP_UINT:  numQuads = indexData->size() / 4; break;
    case OSP_UINT4:
    case OSP_INT4:  numQuads = indexData->size(); break;
    default:
      throw std::runtime_error("unsupported quadmesh.index data type");
    }

    switch (vertexData->type) {
    case OSP_FLOAT:   numVerts = vertexData->size() / 4; numCompsInVtx = 4; break;
    case OSP_FLOAT3:  numVerts = vertexData->size(); numCompsInVtx = 3; break;
    case OSP_FLOAT3A: numVerts = vertexData->size(); numCompsInVtx = 4; break;
    case OSP_FLOAT4 : numVerts = vertexData->size(); numCompsInVtx = 4; break;
    default:
      throw std::runtime_error("unsupported quadmesh.vertex data type");
    }

    if (normalData) switch (normalData->type) {
    case OSP_FLOAT3:  numCompsInNor = 3; break;
    case OSP_FLOAT:
    case OSP_FLOAT3A: numCompsInNor = 4; break;
    default:
      throw std::runtime_error("unsupported quadmesh.vertex.normal data type");
    }

    auto eMeshGeom = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_QUAD);
    rtcSetSharedGeometryBuffer(eMeshGeom,RTC_BUFFER_TYPE_INDEX,0,RTC_FORMAT_UINT4,
                               indexData->data,0,4*sizeof(int),numQuads);
    rtcSetSharedGeometryBuffer(eMeshGeom,RTC_BUFFER_TYPE_VERTEX,0,RTC_FORMAT_FLOAT3,
                               vertexData->data,0,numCompsInVtx*sizeof(int),numVerts);
    rtcCommitGeometry(eMeshGeom);
    eMeshID = rtcAttachGeometry(embreeSceneHandle,eMeshGeom);
    rtcReleaseGeometry(eMeshGeom);

    bounds = empty;

    for (uint32_t i = 0; i < numVerts*numCompsInVtx; i+=numCompsInVtx)
      bounds.extend(*(vec3f*)(vertex + i));

    if (numPrints < 5) {
      postStatusMsg(2) << "  created quad mesh (" << numQuads << " quads "
                       << ", " << numVerts << " vertices)\n"
                       << "  mesh bounds " << bounds;
    }

    ispc::QuadMesh_set(getIE(),model->getIE(),
                       eMeshGeom,
                       eMeshID,
                       numQuads,
                       numCompsInVtx,
                       numCompsInNor,
                       (ispc::vec4i*)index,
                       (float*)vertex,
                       (float*)normal,
                       (ispc::vec4f*)color,
                       (ispc::vec2f*)texcoord,
                       geom_materialID,
                       materialList ? ispcMaterialPtrs.data() : nullptr,
                       (uint32_t*)prim_materialID,
                       colorData && colorData->type == OSP_FLOAT4,
                       huge_mesh);
  }

  OSP_REGISTER_GEOMETRY(QuadMesh,quads);
  OSP_REGISTER_GEOMETRY(QuadMesh,quadmesh);

} // ::ospray
