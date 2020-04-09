// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Spheres.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

Spheres::Spheres()
{
  ispcEquivalent = ispc::Spheres_create(this);
  embreeGeometry = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
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

  ispc::SpheresGeometry_set(getIE(),
      embreeGeometry,
      ispc(vertexData),
      ispc(radiusData),
      ispc(texcoordData),
      radius);

  postCreationInfo();
}

size_t Spheres::numPrimitives() const
{
  return vertexData ? vertexData->size() : 0;
}

} // namespace ospray
