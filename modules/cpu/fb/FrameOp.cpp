// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameOp.h"

namespace ospray {

LiveFrameOp::LiveFrameOp(api::ISPCDevice &device, FrameBufferView &_fbView)
    : device(device), fbViewSh(device.getIspcrtContext())
{
  // Copy data pointed by _fbView pointer to shared memory
  std::memcpy(fbViewSh.data(), &_fbView, sizeof(FrameBufferView));
}

} // namespace ospray
