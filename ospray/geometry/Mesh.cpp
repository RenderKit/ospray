// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Mesh.h"
#include "../include/ospray/ospray.h"
#include "common/World.h"
// ispc exports
#include <cmath>
#include "Mesh_ispc.h"

namespace ospray {

std::string Mesh::toString() const
{
  return "ospray::Mesh";
}

void Mesh::commit()
{
  vertexData = getParamDataT<vec3f>("vertex.position", true);
  normalData = getParamDataT<vec3f>("vertex.normal");
  colorData = getParam<Data *>("vertex.color");
  texcoordData = getParamDataT<vec2f>("vertex.texcoord");
  indexData = getParamDataT<vec3ui>("index");
  if (!indexData)
    indexData = getParamDataT<vec4ui>("index", true);

  postCreationInfo(vertexData->size());
}

LiveGeometry Mesh::createEmbreeGeometry()
{
  const bool isTri = indexData->type == OSP_VEC3UI;

  auto ispcEquivalent = ispc::Mesh_create(this);
  auto embreeGeometry = rtcNewGeometry(ispc_embreeDevice(),
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

  ispc::Mesh_set(ispcEquivalent,
      ispc(indexData),
      ispc(vertexData),
      ispc(normalData),
      ispc(colorData),
      ispc(texcoordData),
      colorData && colorData->type == OSP_VEC4F,
      isTri);

  LiveGeometry retval;

  retval.ispcEquivalent = ispcEquivalent;
  retval.embreeGeometry = embreeGeometry;

  return retval;
}

size_t Mesh::numPrimitives() const
{
  return indexData->size();
}

OSP_REGISTER_GEOMETRY(Mesh, mesh);

} // namespace ospray
