// Copyright 2009-2021 Intel Corporation
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
  if (!embreeDevice) {
    throw std::runtime_error("invalid Embree device");
  }
  if (!embreeGeometry) {
    embreeGeometry = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_USER);
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
