// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#undef NDEBUG

// ospray
#include "Planes.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Planes_ispc.h"

namespace ospray {

Planes::Planes()
{
  embreeGeometry = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
  ispcEquivalent = ispc::Planes_create(this);
}

std::string Planes::toString() const
{
  return "ospray::Planes";
}

void Planes::commit()
{
  planeData = getParamDataT<vec4f>("plane", true);

  ispc::Planes_set(getIE(), embreeGeometry, ispc(planeData));

  postCreationInfo();
}

size_t Planes::numPrimitives() const
{
  return planeData ? planeData->size() : 0;
}

} // namespace ospray
