// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Boxes.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "geometry/Boxes_ispc.h"

namespace ospray {

Boxes::Boxes()
{
  ispcEquivalent = ispc::Boxes_create(this);
}

std::string Boxes::toString() const
{
  return "ospray::Boxes";
}

void Boxes::commit()
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
  boxData = getParamDataT<box3f>("box", true);

  ispc::Boxes_set(getIE(), embreeGeometry, ispc(boxData));

  postCreationInfo();
}

size_t Boxes::numPrimitives() const
{
  return boxData ? boxData->size() : 0;
}

} // namespace ospray
