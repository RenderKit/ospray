// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PixelOp.h"

namespace ospray {

// ImageOp definitions ////////////////////////////////////////////////////////

LivePixelOp::LivePixelOp(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext())
{}

} // namespace ospray
