// Copyright 2019-2020 Intel Corporation
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
  if (!m_device)
  {
    return;
  }
  if (!embreeGeometry)
  {
    ospray::api::ISPCDevice *idev = (ospray::api::ISPCDevice*)m_device;
    embreeGeometry = rtcNewGeometry(idev->ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
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
