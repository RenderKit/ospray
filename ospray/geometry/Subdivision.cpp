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
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    colorsData = getParamDataT<vec4f>("vertex.color");
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");

    level = getParam<float>("level", 5.f);

    indexData = getParamDataT<uint32_t>("index", true);
    indexLevelData = getParamDataT<float>("index.level");

    facesData = getParamDataT<uint32_t>("face");

    edge_crease_indicesData = getParamDataT<vec2ui>("edgeCrease.index");
    edge_crease_weightsData = getParamDataT<float>("edgeCrease.weight");

    vertex_crease_indicesData = getParamDataT<uint32_t>("vertexCrease.index");
    vertex_crease_weightsData = getParamDataT<float>("vertexCrease.weight");

    if (!facesData) {
      if (indexData->size() % 4 != 0)
        throw std::runtime_error(toString()
            + ": if no 'face' array is present then a pure quad mesh is assumed (the number of indices must be a multiple of 4)");

      auto data = new Data(OSP_UINT, vec3ui(indexData->size() / 4, 1, 1));
      facesData = &(data->as<uint32_t>());
      data->refDec();
      for (auto &&face : *facesData)
        face = 4;
    }

    postCreationInfo(vertexData->size());
  }

  size_t Subdivision::numPrimitives() const
  {
    return facesData->size();
  }

  LiveGeometry Subdivision::createEmbreeGeometry()
  {
    auto embreeGeo =
        rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_SUBDIVISION);

    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_VERTEX, vertexData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_INDEX, indexData);
    setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_FACE, facesData);

    if (edge_crease_indicesData && edge_crease_weightsData) {
      if (edge_crease_indicesData->size() != edge_crease_weightsData->size())
        postStatusMsg(1) << toString()
                + " ignoring edge creases, because size of arrays 'edgeCrease.index' and 'edgeCrease.weight' does not match";
      else {
        setEmbreeGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_EDGE_CREASE_INDEX,
            edge_crease_indicesData);
        setEmbreeGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT,
            edge_crease_weightsData);
      }
    }

    if (vertex_crease_indicesData && vertex_crease_weightsData) {
      if (vertex_crease_indicesData->size()
          != vertex_crease_weightsData->size())
        postStatusMsg(1) << toString()
                + " ignoring vertex creases, because size of arrays 'vertexCrease.index' and 'vertexCrease.weight' does not match";
      else {
        setEmbreeGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,
            vertex_crease_indicesData);
        setEmbreeGeometryBuffer(embreeGeo,
            RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT,
            vertex_crease_weightsData);
      }
    }

    if (colorsData) {
      rtcSetGeometryVertexAttributeCount(embreeGeo, 1);
      setEmbreeGeometryBuffer(
          embreeGeo, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, colorsData);
    }

    if (texcoordData) {
      rtcSetGeometryVertexAttributeCount(embreeGeo, 2);
      setEmbreeGeometryBuffer(
          embreeGeo, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, texcoordData, 1);
    }

    if (!indexLevelData)
      rtcSetGeometryTessellationRate(embreeGeo, level);
    else {
      setEmbreeGeometryBuffer(embreeGeo, RTC_BUFFER_TYPE_LEVEL, indexLevelData);
    }

    rtcCommitGeometry(embreeGeo);

    LiveGeometry retval;
    retval.ispcEquivalent = ispc::Subdivision_create(this);
    retval.embreeGeometry = embreeGeo;

    ispc::Subdivision_set(
        retval.ispcEquivalent, retval.embreeGeometry, colorsData, texcoordData);

    return retval;
  }

  OSP_REGISTER_GEOMETRY(Subdivision, subdivision);

}  // namespace ospray
