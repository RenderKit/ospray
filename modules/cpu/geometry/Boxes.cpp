// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Boxes.h"
#include "common/Data.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc-generated files
#include "geometry/Boxes_ispc.h"
#else
namespace ispc {
void Boxes_bounds(const RTCBoundsFunctionArguments *uniform args);
}
#endif

namespace ospray {

Boxes::Boxes(api::ISPCDevice &device)
    : AddStructShared(
        device.getIspcrtContext(), device, FFG_BOX | FFG_USER_GEOMETRY)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.postIntersect =
      reinterpret_cast<ispc::Geometry_postIntersectFct>(
          ispc::Boxes_postIntersect_addr());
#endif
}

std::string Boxes::toString() const
{
  return "ospray::Boxes";
}

void Boxes::commit()
{
  boxData = getParamDataT<box3f>("box", true);

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Boxes_bounds);
  getSh()->boxes = *ispc(boxData);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo();
}

size_t Boxes::numPrimitives() const
{
  return boxData ? boxData->size() : 0;
}

} // namespace ospray
