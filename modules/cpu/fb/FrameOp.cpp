// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameOp.h"

namespace ospray {

LiveFrameOp::LiveFrameOp(devicert::Device &device, FrameBufferView &fbView)
    : AddStructShared(device), device(device)
{
  // We need `FrameBufferView` to be in `ospray` namespace for external modules
  // to reach it, and we need it to be in `ispc` namespace for ISPC automatic
  // headers generator, copying between those two requires a cast
  *getSh() = *reinterpret_cast<ispc::FrameBufferView *>(&fbView);
}

} // namespace ospray
