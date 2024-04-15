// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Mesh.h"
#include "common/DGEnum.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "geometry/Mesh_ispc.h"
#else
namespace ispc {
void *QuadMesh_postIntersect_addr();
void *TriangleMesh_postIntersect_addr();
void *Mesh_sampleArea_addr();
void *Mesh_getAreas_addr();
} // namespace ispc
#endif
// std
#include <numeric>

namespace ospray {

Mesh::Mesh(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device, FFG_NONE)
{
  getSh()->super.getAreas =
      reinterpret_cast<ispc::Geometry_GetAreasFct>(ispc::Mesh_getAreas_addr());
  getSh()->super.sampleArea = reinterpret_cast<ispc::Geometry_SampleAreaFct>(
      ispc::Mesh_sampleArea_addr());
}

std::string Mesh::toString() const
{
  return "ospray::Mesh";
}

void Mesh::commit()
{
  bool isNormalFaceVarying = true;

  motionVertexData =
      getParamDataT<const DataT<vec3f> *>("motion.vertex.position");
  if (motionVertexData) {
    const auto mKeys = motionVertexData->size();
    motionBlur = mKeys > 1;
    if (!motionVertexAddr || motionVertexAddr->size() < mKeys) // reallocate?
      motionVertexAddr = devicert::make_buffer_shared_unique<vec3f *>(
          getISPCDevice().getDRTDevice(), mKeys);
    auto mVAddr = motionVertexAddr->begin();
    // check types and strides
    vertexData = (*motionVertexData)[0]; // use 1st key as fallback
    const int64_t stride = vertexData->ispc.byteStride;
    const size_t size = vertexData->ispc.numItems;
    for (size_t i = 0; i < mKeys; i++) {
      auto &&vtxData = (*motionVertexData)[i];
      if (vtxData->type != OSP_VEC3F || vtxData->ispc.numItems != size
          || vtxData->ispc.byteStride != stride)
        throw std::runtime_error(
            "Mesh 'motion.vertex.position' arrays need to be"
            " of same size and stride and have type vec3f");
      mVAddr[i] = vtxData->data();
    }
    motionNormalData = getParamDataT<const DataT<vec3f> *>("motion.normal");
    if (!motionNormalData) {
      motionNormalData =
          getParamDataT<const DataT<vec3f> *>("motion.vertex.normal");
      isNormalFaceVarying = false;
    }
    if (motionNormalData) {
      if (motionNormalData->size() < mKeys)
        throw std::runtime_error(
            "Mesh 'motion*.normal' array has less keys than"
            " 'motion.vertex.position'");
      if (!motionNormalAddr || motionNormalAddr->size() < mKeys) // reallocate?
        motionNormalAddr = devicert::make_buffer_shared_unique<vec3f *>(
            getISPCDevice().getDRTDevice(), mKeys);
      auto mNAddr = motionNormalAddr->begin();
      // check types and strides
      normalData = (*motionNormalData)[0]; // use 1st key as fallback
      const int64_t stride = normalData->ispc.byteStride;
      for (size_t i = 0; i < mKeys; i++) {
        auto &&norData = (*motionNormalData)[i];
        if (norData->type != OSP_VEC3F || norData->ispc.numItems != size
            || norData->ispc.byteStride != stride)
          throw std::runtime_error(
              "Mesh 'motion*.normal' arrays need to be"
              " of same size and stride and have type vec3f");
        mNAddr[i] = norData->data();
      }
    } else
      normalData = nullptr;
  } else {
    motionBlur = false;
    vertexData = getParamDataT<vec3f>("vertex.position", true);
    normalData = getParamDataT<vec3f>("normal");
    if (!normalData) {
      normalData = getParamDataT<vec3f>("vertex.normal");
      isNormalFaceVarying = false;
    }
  }

  colorData = getParamObject<Data>("color");
  bool isColorFaceVarying = true;
  if (!colorData) {
    colorData = getParamObject<Data>("vertex.color");
    isColorFaceVarying = false;
  }
  bool isTexcoordFaceVarying = true;
  texcoordData = getParamDataT<vec2f>("texcoord");
  if (!texcoordData) {
    texcoordData = getParamDataT<vec2f>("vertex.texcoord");
    isTexcoordFaceVarying = false;
  }

  // get optional vec3i index, if successful, it's a triangle
  indexData = getParamDataT<vec3ui>("index");

  // optionally get vec4i index, if successful, it's a quad
  if (!indexData)
    indexData = getParamDataT<vec4ui>("index");

  // If no index data given, make tri or quad soup based on input parameter
  // Index order is 0,1,2,3,...
  if (!indexData) {
    const bool quadSoup = getParam<bool>("quadSoup", false);
    const int elements = quadSoup ? 4 : 3;
    indexData = new Data(getISPCDevice(),
        quadSoup ? OSP_VEC4UI : OSP_VEC3UI,
        vec3l(vertexData->size() / elements, 1, 1));
    indexData->refDec();
    auto begin = (unsigned int *)indexData->data();
    std::iota(begin, begin + indexData->size() * elements, 0);
  }

  const bool isTri = indexData->type == OSP_VEC3UI;

  getSh()->super.type =
      isTri ? ispc::GEOMETRY_TYPE_TRIANGLE_MESH : ispc::GEOMETRY_TYPE_QUAD_MESH;

  createEmbreeGeometry(
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

  getSh()->isColorFaceVarying = isColorFaceVarying;
  getSh()->isTexcoordFaceVarying = isTexcoordFaceVarying;
  getSh()->isNormalFaceVarying = isNormalFaceVarying;
  getSh()->index = *ispc(indexData);
  getSh()->vertex = *ispc(vertexData);
  getSh()->normal = *ispc(normalData);
  getSh()->color = *ispc(colorData);
  getSh()->texcoord = *ispc(texcoordData);
  getSh()->motionVertex =
      motionVertexAddr ? (uint8_t **)motionVertexAddr->data() : nullptr;
  getSh()->motionNormal =
      motionNormalAddr ? (uint8_t **)motionNormalAddr->data() : nullptr;
  getSh()->motionKeys = motionBlur ? motionVertexData->size() : 0;
  getSh()->time = time;
  getSh()->has_alpha = colorData && colorData->type == OSP_VEC4F;
  getSh()->is_triangleMesh = isTri;
  getSh()->super.numPrimitives = numPrimitives();
  getSh()->super.postIntersect = isTri
      ? reinterpret_cast<ispc::Geometry_postIntersectFct>(
          ispc::TriangleMesh_postIntersect_addr())
      : reinterpret_cast<ispc::Geometry_postIntersectFct>(
          ispc::QuadMesh_postIntersect_addr());

  getSh()->flagMask = -1;
  if (!normalData)
    getSh()->flagMask &= ispc::int64(~DG_NS);
  if (!colorData)
    getSh()->flagMask &= ispc::int64(~DG_COLOR);
  if (!texcoordData)
    getSh()->flagMask &= ispc::int64(~DG_TEXCOORD);

  postCreationInfo(vertexData->size());
  featureFlagsGeometry = isTri ? FFG_TRIANGLE : FFG_QUAD;
  if (motionBlur)
    featureFlagsGeometry |= FFG_MOTION_BLUR;
}

size_t Mesh::numPrimitives() const
{
  return indexData->size();
}

} // namespace ospray
