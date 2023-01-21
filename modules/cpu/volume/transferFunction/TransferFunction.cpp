// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "TransferFunction.h"

namespace ospray {

// TransferFunction definitions ///////////////////////////////////////////////

TransferFunction::TransferFunction(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{
  managedObjectType = OSP_TRANSFER_FUNCTION;
}

void TransferFunction::commit()
{
  auto param = getParam<vec2f>("valueRange", vec2f(0.0f, 1.0f));
  valueRange = getParam<range1f>("value", range1f(param.x, param.y));
  getSh()->valueRange = valueRange;
}

std::string TransferFunction::toString() const
{
  return "ospray::TransferFunction";
}

OSPTYPEFOR_DEFINITION(TransferFunction *);

} // namespace ospray
