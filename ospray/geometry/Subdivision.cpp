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
    auto edge_crease_indicesData = getParamData("edgeCrease.index");
    auto edge_crease_weightsData = getParamData("edgeCrease.weight");
    auto vertex_crease_indicesData = getParamData("vertexCrease.index");
    auto vertex_crease_weightsData = getParamData("vertexCrease.weight");
    auto colorsData = getParamData("vertex.color",getParamData("color"));
    auto texcoordData = getParamData("vertex.texcoord",getParamData("texcoord"));
    auto level = getParam1f("level", 5.f);
    auto indexLevelData = getParamData("index.level");
    auto prim_materialIDData = getParamData("prim.materialID");
    auto geom_materialID = getParam1i("geom.materialID",-1);

    //check for valid params
    if (!vertexData || vertexData->type != OSP_FLOAT3)
      throw std::runtime_error("subdivision must have 'vertex' array of type float3");
    if (!indexData)
      throw std::runtime_error("subdivision must have 'index' array");

    size_t numFaces = 0;
    uint32_t* faces = nullptr;
    if (facesData) {
      if (indexData->type != OSP_INT && indexData->type != OSP_UINT)
        throw std::runtime_error("subdivision 'face' data type must be (u)int");
      generatedFacesData.clear();
      numFaces = facesData->size();
      faces = (uint32_t*)facesData->data;
    } else {
      if (indexData->type != OSP_INT4 && indexData->type != OSP_UINT4)
        throw std::runtime_error("subdivision must have 'face' array or (u)int4 'index'");
      // if face is not specified and index is of type (u)int4, a quad cage mesh is specified
      numFaces = indexData->size()/4;
      generatedFacesData.resize(numFaces, 4);
      faces = generatedFacesData.data();
    }

    if (colorsData && colorsData->type != OSP_FLOAT4)
      throw std::runtime_error("unsupported subdivision 'vertex.color' data type");
    if (texcoordData && texcoordData->type != OSP_FLOAT2)
      throw std::runtime_error("unsupported subdivision 'vertex.texcoord' data type");

    vec3f* vertex = (vec3f*)vertexData->data;
    float* colors = colorsData ?(float*)colorsData->data : nullptr;
    float* indexLevel = indexLevelData ? (float*)indexLevelData->data : nullptr;
    uint32_t* prim_materialID  = prim_materialIDData ? (uint32_t*)prim_materialIDData->data : nullptr;
    vec2f* texcoord = texcoordData ? (vec2f*)texcoordData->data : nullptr;

    auto geom = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_SUBDIVISION);

    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                               vertex, 0, sizeof(vec3f), vertexData->size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT,
        indexData->data, 0, sizeof(unsigned int), indexData->size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_FACE, 0, RTC_FORMAT_UINT,
        faces, 0, sizeof(unsigned int), numFaces);

    if (edge_crease_indicesData && edge_crease_weightsData) {
      size_t edge_creases = edge_crease_indicesData->size();
      if (edge_creases != edge_crease_weightsData->size())
        postStatusMsg(1) << "subdivision edge crease indices size does not match weights";
      else
      {
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_EDGE_CREASE_INDEX, 0,
            RTC_FORMAT_UINT2, edge_crease_indicesData->data, 0,
            2*sizeof(unsigned int), edge_creases);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT, 0,
            RTC_FORMAT_FLOAT, edge_crease_weightsData->data, 0, sizeof(float),
            edge_creases);
      }
    }

    if (vertex_crease_indicesData && vertex_crease_weightsData) {
      size_t vertex_creases = vertex_crease_indicesData->size();
      if (vertex_creases != vertex_crease_weightsData->size())
        postStatusMsg(1) << "subdivision vertex crease indices size does not match weights";
      else
      {
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,
            0, RTC_FORMAT_UINT, vertex_crease_indicesData->data, 0,
            sizeof(unsigned int), vertex_creases);
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT,
            0, RTC_FORMAT_FLOAT, vertex_crease_weightsData->data, 0,
            sizeof(float), vertex_creases);
      }
    }

    if (colors) {
      rtcSetGeometryVertexAttributeCount(geom,1);
      rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
          RTC_FORMAT_FLOAT4, colors, 0, sizeof(vec4f), colorsData->size());
    }

    if (texcoord) {
      rtcSetGeometryVertexAttributeCount(geom,2);
      rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
          RTC_FORMAT_FLOAT2, texcoord, 0, sizeof(vec2f), texcoordData->size());
    }

    if (!indexLevelData)
      rtcSetGeometryTessellationRate(geom, level);
    else {
      rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_LEVEL, 0, RTC_FORMAT_FLOAT, indexLevel,
                                 0, sizeof(float), indexLevelData->size());
    }

    rtcCommitGeometry(geom);
    auto eGeomID = rtcAttachGeometry(model->embreeSceneHandle, geom);
    rtcReleaseGeometry(geom);

    bounds = empty;

    /* better bounds if some vertices are not referenced:
    for (auto i : indexData)
      bounds.extend(vertexData[i]); */
    for (size_t i = 0; i < vertexData->size(); i++)
      bounds.extend(vertex[i]);
    //TODO: must factor in displacement into bounds....


    postStatusMsg(2) << "  created subdivision (" << numFaces << " faces "
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
