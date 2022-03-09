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

  vertexData = getParamDataT<vec3f>("vertex.position", true);
  normalData = getParamDataT<vec3f>("vertex.normal");
  colorData = getParam<Data *>("vertex.color");
  texcoordData = getParamDataT<vec2f>("vertex.texcoord");

  tex1dData = getParamDataT<float>("vertex.texcoord1d");
  tex1dData_elm = getParamDataT<float>("vertex.texcoord1d_elm");
  atex1dData = getParamDataT<float>("vertex.alphatexcoord1d");
  atex1dData_elm = getParamDataT<float>("vertex.alphatexcoord1d_elm");

  indexData = getParamDataT<vec3ui>("index");
  if (!indexData)
    indexData = getParamDataT<vec4ui>("index", true);

  const bool isTri = indexData->type == OSP_VEC3UI;

  embreeGeometry = rtcNewGeometry(embreeDevice,
      isTri ? RTC_GEOMETRY_TYPE_TRIANGLE : RTC_GEOMETRY_TYPE_QUAD);
  setEmbreeGeometryBuffer(embreeGeometry, RTC_BUFFER_TYPE_VERTEX, vertexData);
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
      ispc(tex1dData),
      ispc(tex1dData_elm),
      ispc(atex1dData),
      ispc(atex1dData_elm),
      colorData && colorData->type == OSP_VEC4F,
      isTri);

  postCreationInfo(vertexData->size());
}

size_t Mesh::numPrimitives() const
{
  return indexData->size();
}

} // namespace ospray
