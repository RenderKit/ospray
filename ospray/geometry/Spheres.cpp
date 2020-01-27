// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#undef NDEBUG

// ospray
#include "Spheres.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

Spheres::Spheres()
{
  // no-op
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

  postCreationInfo();
}

LiveGeometry Spheres::createEmbreeGeometry()
{
  LiveGeometry retval;

  retval.ispcEquivalent = ispc::Spheres_create(this);
  retval.embreeGeometry =
      rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);

  ispc::SpheresGeometry_set(retval.ispcEquivalent,
      retval.embreeGeometry,
      ispc(vertexData),
      ispc(radiusData),
      ispc(texcoordData),
      radius);

  return retval;
}

size_t Spheres::numPrimitives() const
{
  return vertexData ? vertexData->size() : 0;
}

OSP_REGISTER_GEOMETRY(Spheres, sphere);

} // namespace ospray
