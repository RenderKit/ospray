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
#include "Subdivision.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "Subdivision_ispc.h"

namespace ospray {

  std::string Subdivision::toString() const
  {
    return "ospray::Subdivision";
  }

  void Subdivision::commit()
  {
    vertexData   = getParamData("vertex.position");
    colorsData   = getParamData("vertex.color");
    texcoordData = getParamData("vertex.texcoord");

    level = getParam1f("level", 5.f);

    indexData      = getParamData("index");
    indexLevelData = getParamData("index.level");

    facesData = getParamData("face");

    edge_crease_indicesData = getParamData("edgeCrease.index");
    edge_crease_weightsData = getParamData("edgeCrease.weight");

    vertex_crease_indicesData = getParamData("vertexCrease.index");
    vertex_crease_weightsData = getParamData("vertexCrease.weight");

    // check for valid params
    if (!vertexData || vertexData->type != OSP_VEC3F)
      throw std::runtime_error(
          "subdivision must have 'vertex' array of type float3");
    if (!indexData)
      throw std::runtime_error("subdivision must have 'index' array");

    faces = nullptr;
    if (facesData) {
      if (indexData->type != OSP_INT && indexData->type != OSP_UINT)
        throw std::runtime_error("subdivision 'face' data type must be (u)int");
      generatedFacesData.clear();
      numFaces = facesData->size();
      faces    = (uint32_t *)facesData->data;
    } else {
      if (indexData->type != OSP_VEC4I && indexData->type != OSP_VEC4UI)
        throw std::runtime_error(
            "subdivision must have 'face' array or (u)int4 'index'");
      // if face is not specified and index is of type (u)int4, a quad cage mesh
      // is specified
      numFaces = indexData->size() / 4;
      generatedFacesData.resize(numFaces, 4);
      faces = generatedFacesData.data();
    }

    if (colorsData && colorsData->type != OSP_VEC4F)
      throw std::runtime_error(
          "unsupported subdivision 'vertex.color' data type");
    if (texcoordData && texcoordData->type != OSP_VEC2F)
      throw std::runtime_error(
          "unsupported subdivision 'vertex.texcoord' data type");

    postStatusMsg(2) << "  created subdivision (" << numFaces << " faces "
                     << ", " << vertexData->size() << " vertices)\n";
  }

  size_t Subdivision::numPrimitives() const
  {
    return indexData ? indexData->numItems / 4 : 0;
  }

  LiveGeometry Subdivision::createEmbreeGeometry()
  {
    LiveGeometry retval;

    retval.ispcEquivalent = ispc::Subdivision_create(this);
    retval.embreeGeometry =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_SUBDIVISION);

    vec3f *vertex = (vec3f *)vertexData->data;
    float *colors = colorsData ? (float *)colorsData->data : nullptr;
    float *indexLevel =
        indexLevelData ? (float *)indexLevelData->data : nullptr;
    vec2f *texcoord = texcoordData ? (vec2f *)texcoordData->data : nullptr;

    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_VERTEX,
                               0,
                               RTC_FORMAT_FLOAT3,
                               vertex,
                               0,
                               sizeof(vec3f),
                               vertexData->size());
    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_INDEX,
                               0,
                               RTC_FORMAT_UINT,
                               indexData->data,
                               0,
                               sizeof(unsigned int),
                               indexData->size());
    rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                               RTC_BUFFER_TYPE_FACE,
                               0,
                               RTC_FORMAT_UINT,
                               faces,
                               0,
                               sizeof(unsigned int),
                               numFaces);

    if (edge_crease_indicesData && edge_crease_weightsData) {
      size_t edge_creases = edge_crease_indicesData->size();
      if (edge_creases != edge_crease_weightsData->size())
        postStatusMsg(1)
            << "subdivision edge crease indices size does not match weights";
      else {
        rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                   RTC_BUFFER_TYPE_EDGE_CREASE_INDEX,
                                   0,
                                   RTC_FORMAT_UINT2,
                                   edge_crease_indicesData->data,
                                   0,
                                   2 * sizeof(unsigned int),
                                   edge_creases);
        rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                   RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT,
                                   0,
                                   RTC_FORMAT_FLOAT,
                                   edge_crease_weightsData->data,
                                   0,
                                   sizeof(float),
                                   edge_creases);
      }
    }

    if (vertex_crease_indicesData && vertex_crease_weightsData) {
      size_t vertex_creases = vertex_crease_indicesData->size();
      if (vertex_creases != vertex_crease_weightsData->size())
        postStatusMsg(1)
            << "subdivision vertex crease indices size does not match weights";
      else {
        rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                   RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,
                                   0,
                                   RTC_FORMAT_UINT,
                                   vertex_crease_indicesData->data,
                                   0,
                                   sizeof(unsigned int),
                                   vertex_creases);
        rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                   RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT,
                                   0,
                                   RTC_FORMAT_FLOAT,
                                   vertex_crease_weightsData->data,
                                   0,
                                   sizeof(float),
                                   vertex_creases);
      }
    }

    if (colors) {
      rtcSetGeometryVertexAttributeCount(retval.embreeGeometry, 1);
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                 0,
                                 RTC_FORMAT_FLOAT4,
                                 colors,
                                 0,
                                 sizeof(vec4f),
                                 colorsData->size());
    }

    if (texcoord) {
      rtcSetGeometryVertexAttributeCount(retval.embreeGeometry, 2);
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                 1,
                                 RTC_FORMAT_FLOAT2,
                                 texcoord,
                                 0,
                                 sizeof(vec2f),
                                 texcoordData->size());
    }

    if (!indexLevelData)
      rtcSetGeometryTessellationRate(retval.embreeGeometry, level);
    else {
      rtcSetSharedGeometryBuffer(retval.embreeGeometry,
                                 RTC_BUFFER_TYPE_LEVEL,
                                 0,
                                 RTC_FORMAT_FLOAT,
                                 indexLevel,
                                 0,
                                 sizeof(float),
                                 indexLevelData->size());
    }

    rtcCommitGeometry(retval.embreeGeometry);

    ispc::Subdivision_set(
        retval.ispcEquivalent, retval.embreeGeometry, (ispc::vec2f *)texcoord);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Subdivision, subdivision);

}  // namespace ospray
