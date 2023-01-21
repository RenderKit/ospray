// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Planes.h"
#include "common/Data.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc-generated files
#include "geometry/Planes_ispc.h"
#else
namespace ispc {
void Planes_bounds(const RTCBoundsFunctionArguments *args);
}
#endif

namespace ospray {

Planes::Planes(api::ISPCDevice &device)
    : AddStructShared(
        device.getIspcrtContext(), device, FFG_PLANE | FFG_USER_GEOMETRY)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.postIntersect =
      reinterpret_cast<ispc::Geometry_postIntersectFct>(
          ispc::Planes_postIntersect_addr());
#endif
}

std::string Planes::toString() const
{
  return "ospray::Planes";
}

void Planes::commit()
{
  coeffsData = getParamDataT<vec4f>("plane.coefficients", true);
  boundsData = getParamDataT<box3f>("plane.bounds");

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Planes_bounds);
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
