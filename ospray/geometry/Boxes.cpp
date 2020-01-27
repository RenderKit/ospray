// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Boxes.h"
#include "common/Data.h"
#include "common/World.h"
// ispc-generated files
#include "Boxes_ispc.h"

namespace ospray {

std::string Boxes::toString() const
{
  return "ospray::Boxes";
}

void Boxes::commit()
{
  boxData = getParamDataT<box3f>("box", true);

  postCreationInfo();
}

LiveGeometry Boxes::createEmbreeGeometry()
{
  LiveGeometry retval;

  retval.embreeGeometry =
      rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
  retval.ispcEquivalent = ispc::Boxes_create(this);

  ispc::Boxes_set(retval.ispcEquivalent, retval.embreeGeometry, ispc(boxData));

  return retval;
}

size_t Boxes::numPrimitives() const
{
  return boxData ? boxData->size() : 0;
}

OSP_REGISTER_GEOMETRY(Boxes, box);

} // namespace ospray
