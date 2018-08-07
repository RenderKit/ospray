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
#include "Subdivision.h"
#include "common/Model.h"
#include "../include/ospray/ospray.h"
// ispc exports
#include "Subdivision_ispc.h"
#include <cmath>

namespace ospray {

  Subdivision::Subdivision()
  {
    this->ispcEquivalent = ispc::Subdivision_create(this);
  }

  std::string Subdivision::toString() const
  {
    return "ospray::Subdivision";
  }

  void Subdivision::finalize(Model *model)
  {
    Assert(model && "invalid model pointer");
    Geometry::finalize(model);

    auto vertexData = getParamData("vertex",getParamData("position"));
    auto indexData = getParamData("index");
    auto facesData = getParamData("face");
    auto edge_crease_indicesData = getParamData("edge_crease_indices");
    auto edge_crease_weightsData = getParamData("edge_crease_weights");
    auto vertex_crease_indicesData = getParamData("vertex_crease_indices");
    auto vertex_crease_weightsData = getParamData("vertex_crease_weights");
    auto colorsData = getParamData("vertex.color",getParamData("color"));
    auto texcoordData = getParamData("vertex.texcoord",getParamData("texcoord"));
    auto edgeLevel = getParam1f("edgeLevel", 0.f);
    auto prim_materialIDData = getParamData("prim.materialID");
    auto geom_materialID = getParam1i("geom.materialID",-1);

    int* index = (int*)indexData->data;
    float* vertex = (float*)vertexData->data;
    float* colors = (float*)colorsData->data;
    int* faces = (int*)facesData->data;
    vec2i* edge_crease_indices = (vec2i*)edge_crease_indicesData->data;
    float* edge_crease_weights = (float*)edge_crease_weightsData->data;
    int* vertex_crease_indices = (int*)vertex_crease_indicesData->data;
    float* vertex_crease_weights = (float*)vertex_crease_weightsData->data;
    uint32_t* prim_materialID  = prim_materialIDData ? (uint32_t*)prim_materialIDData->data : nullptr;
    vec2f* texcoord = texcoordData ? (vec2f*)texcoordData->data : nullptr;

    if (vertexData->type != OSP_FLOAT3)
      throw std::runtime_error("unsupported subdivision.vertex data type");

    if (colorsData && colorsData->type != OSP_FLOAT4)
      throw std::runtime_error("unsupported subdivision.colors data type");

    if (indexData->type != OSP_INT && indexData->type != OSP_UINT)
      throw std::runtime_error("unsupported subdivision.index data type");

    auto geom = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_SUBDIVISION);

    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                               vertex, 0, sizeof(vec3f), vertexData->size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT,
                               index, 0, sizeof(unsigned int), indexData->size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_FACE, 0, RTC_FORMAT_UINT,
                               faces, 0, sizeof(unsigned int), facesData->size());

    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_EDGE_CREASE_INDEX,    0,
                               RTC_FORMAT_UINT2, edge_crease_indices,   0, 2*sizeof(unsigned int), 0);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT,   0,
                               RTC_FORMAT_FLOAT, edge_crease_weights,   0, sizeof(float), 0);

    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,  0,
                               RTC_FORMAT_UINT,  vertex_crease_indices, 0, sizeof(unsigned int), 0);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT, 0,
                               RTC_FORMAT_FLOAT, vertex_crease_weights, 0, sizeof(float), 0);

    rtcSetGeometryVertexAttributeCount(geom,1);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                               RTC_FORMAT_FLOAT4, colors, 0, sizeof(vec4f), colorsData->size());
    if (texcoord) {
      rtcSetGeometryVertexAttributeCount(geom,2);
      rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
                                 RTC_FORMAT_FLOAT2, texcoord, 0, sizeof(vec2f), texcoordData->size());
    }

    float* level = (float*) rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_LEVEL, 0,
                               RTC_FORMAT_FLOAT, sizeof(float), indexData->size());
    for (unsigned int i=0; i<indexData->size(); i++)
      level[i] = edgeLevel;

    rtcCommitGeometry(geom);
    auto eGeomID = rtcAttachGeometry(model->embreeSceneHandle, geom);
    rtcReleaseGeometry(geom);

    bounds = empty;

    for (uint32_t i = 0; i < vertexData->size()*3; i+=3)
      bounds.extend(*(vec3f*)((float *)vertexData->data + i));
    //TODO: must factor in displacement into bounds....


    postStatusMsg(2) << "  created subdivision (" << facesData->size() << " faces "
                     << ", " << vertexData->size() << " vertices)\n"
                     << "  mesh bounds " << bounds;

    ispc::Subdivision_set(getIE(), model->getIE(),
                          geom,
                          eGeomID,
                          geom_materialID,
                          prim_materialID,
                          materialList ? ispcMaterialPtrs.data() : nullptr,
                          (ispc::vec2f*)texcoord
                                         );
  }

  OSP_REGISTER_GEOMETRY(Subdivision,subdivision);

} // ::ospray
