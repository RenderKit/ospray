// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Subdivision.h"
#include "common/DGEnum.h"

// ispc exports
#include <cmath>
#include "geometry/Subdivision_ispc.h"

namespace ospray {

Subdivision::Subdivision()
{
  getSh()->super.postIntersect = ispc::Subdivision_postIntersect_addr();
}

std::string Subdivision::toString() const
{
  return "ospray::Subdivision";
}

void Subdivision::commit()
{
  createEmbreeGeometry(RTC_GEOMETRY_TYPE_SUBDIVISION);

  vertexData = getParamDataT<vec3f>("vertex.position", true);
  colorsData = getParamDataT<vec4f>("color");
  bool isColorsFaceVarying = true;
  if (!colorsData) {
    colorsData = getParamDataT<vec4f>("vertex.color");
    isColorsFaceVarying = false;
  }
  texcoordData = getParamDataT<vec2f>("texcoord");
  bool isTexcoordFaceVarying = true;
  if (!texcoordData) {
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");
    isTexcoordFaceVarying = false;
  }

  level = getParam<float>("level", 5.f);

  indexData = getParamDataT<uint32_t>("index", true);
  indexLevelData = getParamDataT<float>("index.level");

  facesData = getParamDataT<uint32_t>("face");

  edge_crease_indicesData = getParamDataT<vec2ui>("edgeCrease.index");
  edge_crease_weightsData = getParamDataT<float>("edgeCrease.weight");

  vertex_crease_indicesData = getParamDataT<uint32_t>("vertexCrease.index");
  vertex_crease_weightsData = getParamDataT<float>("vertexCrease.weight");

  mode = (OSPSubdivisionMode)getParam<uint8_t>(
      "mode", getParam<int32_t>("mode", OSP_SUBDIVISION_SMOOTH_BOUNDARY));

  if (!facesData) {
    if (indexData->size() % 4 != 0)
      throw std::runtime_error(
            toString() +
            ": if no 'face' array is present then a pure quad mesh is assumed "
            "(the number of indices must be a multiple of 4)");

    auto data = new Data(OSP_UINT, vec3ui(indexData->size() / 4, 1, 1));
    facesData = &(data->as<uint32_t>());
    data->refDec();
    for (auto &&face : *facesData)
      face = 4;
  }

  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vertexData);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_INDEX, indexData);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_FACE, facesData);

  rtcSetGeometrySubdivisionMode(embreeGeometry, 0, (RTCSubdivisionMode)mode);

  if (edge_crease_indicesData && edge_crease_weightsData) {
    if (edge_crease_indicesData->size() != edge_crease_weightsData->size())
      postStatusMsg(OSP_LOG_DEBUG)
            << toString() +
                   " ignoring edge creases, because size of arrays "
                   "'edgeCrease.index' and 'edgeCrease.weight' does not match";
    else {
      setEmbreeGeometryBuffer(embreeGeometry,
          RTC_BUFFER_TYPE_EDGE_CREASE_INDEX,
          edge_crease_indicesData);
      setEmbreeGeometryBuffer(embreeGeometry,
          RTC_BUFFER_TYPE_EDGE_CREASE_WEIGHT,
          edge_crease_weightsData);
    }
  }

  if (vertex_crease_indicesData && vertex_crease_weightsData) {
    if (vertex_crease_indicesData->size()
        != vertex_crease_weightsData->size()) {
      postStatusMsg(OSP_LOG_DEBUG)
            << toString() +
                   " ignoring vertex creases, because size of "
                   "arrays 'vertexCrease.index' and "
                   "'vertexCrease.weight' does not match";
    } else {
      setEmbreeGeometryBuffer(embreeGeometry,
          RTC_BUFFER_TYPE_VERTEX_CREASE_INDEX,
          vertex_crease_indicesData);
      setEmbreeGeometryBuffer(embreeGeometry,
          RTC_BUFFER_TYPE_VERTEX_CREASE_WEIGHT,
          vertex_crease_weightsData);
    }
  }

  if ((colorsData && isColorsFaceVarying)
      || (texcoordData && isTexcoordFaceVarying)) {
    if (generatedIndicesSize != indexData->size()) {
      auto data = new Data(OSP_UINT, vec3ui(indexData->size(), 1, 1));
      generatedIndicesData = &(data->as<uint32_t>());
      data->refDec();
      uint32_t *dataptr = static_cast<uint32_t *>(generatedIndicesData->data());
      for (unsigned int i = 0; i < indexData->size(); i++)
        *(dataptr++) = i;
      generatedIndicesSize = indexData->size();
    }
    rtcSetGeometryTopologyCount(embreeGeometry, 2);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_INDEX, generatedIndicesData, 1);
    rtcSetGeometrySubdivisionMode(embreeGeometry, 1, (RTCSubdivisionMode)mode);
  }

  if (colorsData) {
    rtcSetGeometryVertexAttributeCount(embreeGeometry, 1);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, colorsData, 0);
    if (isColorsFaceVarying)
      rtcSetGeometryVertexAttributeTopology(embreeGeometry, 0, 1);
  }

  if (texcoordData) {
    rtcSetGeometryVertexAttributeCount(embreeGeometry, 2);
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, texcoordData, 1);
    if (isTexcoordFaceVarying)
      rtcSetGeometryVertexAttributeTopology(embreeGeometry, 1, 1);
  }

  if (!indexLevelData)
    rtcSetGeometryTessellationRate(embreeGeometry, level);
  else {
    setEmbreeGeometryBuffer(
        embreeGeometry, RTC_BUFFER_TYPE_LEVEL, indexLevelData);
  }

  rtcCommitGeometry(embreeGeometry);

  getSh()->geom = embreeGeometry;
  getSh()->flagMask = -1;
  if (!colorsData)
    getSh()->flagMask &= ispc::int64(~DG_COLOR);
  if (!texcoordData)
    getSh()->flagMask &= ispc::int64(~DG_TEXCOORD);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo(vertexData->size());
}

size_t Subdivision::numPrimitives() const
{
  return facesData->size();
}

} // namespace ospray
