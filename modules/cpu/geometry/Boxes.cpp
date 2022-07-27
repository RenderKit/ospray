// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Boxes.h"
#include "common/Data.h"
// ispc-generated files
#include "geometry/Boxes_ispc.h"

namespace ospray {

Boxes::Boxes()
{
  getSh()->super.postIntersect = ispc::Boxes_postIntersect_addr();
}

std::string Boxes::toString() const
{
  return "ospray::Boxes";
}

void Boxes::commit()
{
  boxData = getParamDataT<box3f>("box", true);

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Boxes_bounds,
      (RTCIntersectFunctionN)&ispc::Boxes_intersect,
      (RTCOccludedFunctionN)&ispc::Boxes_occluded);
  getSh()->boxes = *ispc(boxData);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo();
}

size_t Boxes::numPrimitives() const
{
  return boxData ? boxData->size() : 0;
}

} // namespace ospray
