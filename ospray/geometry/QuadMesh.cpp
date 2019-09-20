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

  std::string QuadMesh::toString() const
  {
    return "ospray::QuadMesh";
  }

  void QuadMesh::commit()
  {
    vertexData   = getParamData("vertex.position");
    normalData   = getParamData("vertex.normal");
    colorData    = getParamData("vertex.color");
    texcoordData = getParamData("vertex.texcoord");
    indexData    = getParamData("index");

    if (!vertexData) {
      throw std::runtime_error(
          "quad mesh geometry must have 'vertex.position' array");
    }

    if (!indexData) {
      throw std::runtime_error("quad mesh geometry must have 'index' array");
    }

    if (colorData && colorData->type != OSP_VEC4F &&
        colorData->type != OSP_VEC3F) {
      std::stringstream ss;
      ss << "quad mesh geometry 'vertex.color' array has invalid element type "
         << stringForType(colorData->type)
         << ". Must be one of: OSP_VEC4F, OSP_VEC3F";
      throw std::runtime_error(ss.str());
    }

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
    case OSP_VEC4UI:
    case OSP_VEC4I:
      numQuads = indexData->size();
      break;
    default: {
      std::stringstream ss;
      ss << "quad mesh geometry 'index' array has invalid element type "
         << stringForType(indexData->type)
         << ". Must be one of: OSP_INT, OSP_UINT, OSP_VEC4UI, OSP_VEC4I";
      throw std::runtime_error(ss.str());
    }
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
    case OSP_VEC4F:
      numVerts      = vertexData->size();
      numCompsInVtx = 4;
      break;
    default: {
      std::stringstream ss;
      ss << "quad mesh geometry 'vertex.position' array has invalid element "
            "type "
         << stringForType(vertexData->type)
         << ". Must be one of: OSP_VEC3F, OSP_VEC4F, OSP_FLOAT";
      throw std::runtime_error(ss.str());
    }
    }

    if (normalData) {
      switch (normalData->type) {
      case OSP_VEC3F:
        numCompsInNor = 3;
        break;
      case OSP_FLOAT:
        numCompsInNor = 4;
        break;
      default: {
        std::stringstream ss;
        ss << "quad mesh geometry 'vertex.normal' array has invalid element type "
           << stringForType(normalData->type)
           << ". Must be one of: OSP_VEC3F, OSP_FLOAT";
        throw std::runtime_error(ss.str());
      }
      }
    }

    postStatusMsg(2) << "  created quad mesh (" << numQuads << " quads, "
                     << numVerts << " vertices)\n";
  }

  size_t QuadMesh::numPrimitives() const
  {
    return numQuads;
  }

  LiveGeometry QuadMesh::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::QuadMesh_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_QUAD);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_INDEX,
                               0,
                               RTC_FORMAT_UINT4,
                               indexData->data,
                               0,
                               4 * sizeof(int),
                               numQuads);

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_VERTEX,
                               0,
                               RTC_FORMAT_FLOAT3,
                               vertexData->data,
                               0,
                               numCompsInVtx * sizeof(int),
                               numVerts);

    rtcCommitGeometry(retval.embreeGeometry);

    ispc::QuadMesh_set(retval.ispcEquivalent,
                       retval.embreeGeometry,
                       numQuads,
                       numCompsInVtx,
                       numCompsInNor,
                       (ispc::vec4i *)index,
                       (float *)vertex,
                       (float *)normal,
                       (ispc::vec4f *)color,
                       (ispc::vec2f *)texcoord,
                       colorData && colorData->type == OSP_VEC4F,
                       huge_mesh);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(QuadMesh, quads);
  OSP_REGISTER_GEOMETRY(QuadMesh, quadmesh);

}  // namespace ospray
