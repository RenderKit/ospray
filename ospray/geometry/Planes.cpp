// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#undef NDEBUG

// ospray
#include "Planes.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "geometry/Planes_ispc.h"

namespace ospray {

Planes::Planes()
{
  ispcEquivalent = ispc::Planes_create(this);
}

std::string Planes::toString() const
{
  return "ospray::Planes";
}

void Planes::commit()
{
  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }
  if (!embreeGeometry) {
    embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_USER);
  }

  coeffsData = getParamDataT<vec4f>("plane.coefficients", true);
  boundsData = getParamDataT<box3f>("plane.bounds");

  ispc::Planes_set(getIE(), embreeGeometry, ispc(coeffsData), ispc(boundsData));

  postCreationInfo();
}

size_t Planes::numPrimitives() const
{
  return coeffsData ? coeffsData->size() : 0;
}

} // namespace ospray
