// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.h"

namespace ospray {

void registerAllTransferFunctions()
{
  TransferFunction::registerType<LinearTransferFunction>("piecewiseLinear");
}

} // namespace ospray
