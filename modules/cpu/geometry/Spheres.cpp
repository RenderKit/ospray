// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Spheres.h"
#include "common/Data.h"
// ispc-generated files
#include "geometry/Spheres_ispc.h"

namespace ospray {

Spheres::Spheres()
{
  getSh()->super.postIntersect = ispc::Spheres_postIntersect_addr();
  getSh()->super.getAreas = ispc::Spheres_getAreas_addr();
  getSh()->super.sampleArea = ispc::Spheres_sampleArea_addr();
}

std::string Spheres::toString() const
{
  return "ospray::Spheres";
}

void Spheres::commit()
{
  radius = getParam<float>("radius", 0.01f);
  vertexData = getParamDataT<vec3f>("sphere.position", true);
  radiusData = getParamDataT<float>("sphere.radius");
  texcoordData = getParamDataT<vec2f>("sphere.texcoord");

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Spheres_bounds,
      (RTCIntersectFunctionN)&ispc::Spheres_intersect,
      (RTCOccludedFunctionN)&ispc::Spheres_occluded);
  getSh()->vertex = *ispc(vertexData);
  getSh()->radius = *ispc(radiusData);
  getSh()->texcoord = *ispc(texcoordData);
  getSh()->global_radius = radius;
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo();
}

size_t Spheres::numPrimitives() const
{
  return vertexData ? vertexData->size() : 0;
}

} // namespace ospray
