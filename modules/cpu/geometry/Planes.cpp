// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#undef NDEBUG

// ospray
#include "Planes.h"
#include "common/Data.h"
// ispc-generated files
#include "geometry/Planes_ispc.h"

namespace ospray {

Planes::Planes()
{
  getSh()->super.postIntersect = ispc::Planes_postIntersect_addr();
}

std::string Planes::toString() const
{
  return "ospray::Planes";
}

void Planes::commit()
{
  coeffsData = getParamDataT<vec4f>("plane.coefficients", true);
  boundsData = getParamDataT<box3f>("plane.bounds");

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Planes_bounds,
      (RTCIntersectFunctionN)&ispc::Planes_intersect,
      (RTCOccludedFunctionN)&ispc::Planes_occluded);
  getSh()->coeffs = *ispc(coeffsData);
  getSh()->bounds = *ispc(boundsData);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo();
}

size_t Planes::numPrimitives() const
{
  return coeffsData ? coeffsData->size() : 0;
}

} // namespace ospray
