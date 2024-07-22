// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "TransferFunction.h"

namespace ospray {

// TransferFunction definitions ///////////////////////////////////////////////

TransferFunction::TransferFunction(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device)
{
  managedObjectType = OSP_TRANSFER_FUNCTION;
}

void TransferFunction::commit()
{
  valueRange = getParam<range1f>("value", range1f(0.0f, 1.0f));
  getSh()->valueRange = valueRange;
}

std::string TransferFunction::toString() const
{
  return "ospray::TransferFunction";
}

OSPTYPEFOR_DEFINITION(TransferFunction *);

} // namespace ospray
