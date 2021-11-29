// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Mesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "geometry/Mesh_ispc.h"

namespace ospray {

Mesh::Mesh()
{
  ispcEquivalent = ispc::Mesh_create();
}

std::string Mesh::toString() const
{
  return "ospray::Mesh";
}

void Mesh::commit()
{
  if (embreeGeometry)
    rtcReleaseGeometry(embreeGeometry);

  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }

  motionVertexAddr.clear();
  motionNormalAddr.clear();

  motionVertexData =
      getParamDataT<const DataT<vec3f> *>("motion.vertex.position");
  if (motionVertexData) {
    motionBlur = motionVertexData->size() > 1;
    // check types and strides
    vertexData = (*motionVertexData)[0]; // use 1st key as fallback
    const int64_t stride = vertexData->ispc.byteStride;
    const size_t size = vertexData->ispc.numItems;
    for (auto &&vtxData : *motionVertexData) {
      if (vtxData->type != OSP_VEC3F || vtxData->ispc.numItems != size
          || vtxData->ispc.byteStride != stride)
        throw std::runtime_error(
            "Mesh 'motion.vertex.position' arrays need to be"
            " of same size and stride and have type vec3f");
      motionVertexAddr.push_back(vtxData->data());
    }
    motionNormalData =
        getParamDataT<const DataT<vec3f> *>("motion.vertex.normal");
    if (motionNormalData) {
      if (motionNormalData->size() < motionVertexData->size())
        throw std::runtime_error(
            "Mesh 'motion.vertex.normal' array has less keys than"
            " 'motion.vertex.position'");
      // check types and strides
      normalData = (*motionNormalData)[0]; // use 1st key as fallback
      const int64_t stride = normalData->ispc.byteStride;
      for (auto &&norData : *motionNormalData) {
        if (norData->type != OSP_VEC3F || norData->ispc.numItems != size
            || norData->ispc.byteStride != stride)
          throw std::runtime_error(
              "Mesh 'motion.vertex.normal' arrays need to be"
              " of same size and stride and have type vec3f");
        motionNormalAddr.push_back(norData->data());
      }
    } else
      normalData = nullptr;
  } else {
    motionBlur = false;
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    normalData = getParamDataT<vec3f>("vertex.normal");
  }

  colorData = getParam<Data *>("vertex.color");
  texcoordData = getParamDataT<vec2f>("vertex.texcoord");
  indexData = getParamDataT<vec3ui>("index");
  if (!indexData)
    indexData = getParamDataT<vec4ui>("index", true);

  const bool isTri = indexData->type == OSP_VEC3UI;

  embreeGeometry = rtcNewGeometry(embreeDevice,
      isTri ? RTC_GEOMETRY_TYPE_TRIANGLE : RTC_GEOMETRY_TYPE_QUAD);

  time = range1f(0.0f, 1.0f);
  if (motionBlur) {
    time = getParam<range1f>("time", range1f(0.0f, 1.0f));
    if (time.upper < time.lower)
      time.upper = time.lower;

    rtcSetGeometryTimeStepCount(embreeGeometry, motionVertexData->size());
    size_t i = 0;
    for (auto &&vtx : *motionVertexData)
      setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vtx, i++);
    rtcSetGeometryTimeRange(embreeGeometry, time.lower, time.upper);
  } else {
    rtcSetGeometryTimeStepCount(embreeGeometry, 1);
    setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vertexData);
  }

  rtcSetSharedGeometryBuffer(embreeGeometry,
      RTC_BUFFER_TYPE_INDEX,
      0,
      isTri ? RTC_FORMAT_UINT3 : RTC_FORMAT_UINT4,
      indexData->data(),
      0,
      isTri ? sizeof(vec3ui) : sizeof(vec4ui),
      indexData->size());
  rtcCommitGeometry(embreeGeometry);

  ispc::Mesh_set(getIE(),
      ispc(indexData),
      ispc(vertexData),
      ispc(normalData),
      ispc(colorData),
      ispc(texcoordData),
      motionVertexAddr.data(),
      motionNormalAddr.data(),
      motionBlur ? motionVertexData->size() : 0,
      (const ispc::box1f &)time,
      colorData && colorData->type == OSP_VEC4F,
      isTri);

  postCreationInfo(vertexData->size());
}

size_t Mesh::numPrimitives() const
{
  return indexData->size();
}

} // namespace ospray
