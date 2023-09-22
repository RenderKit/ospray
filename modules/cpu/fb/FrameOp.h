// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/StructShared.h"
#include "fb/ImageOp.h"
// ispc shared
#include "fb/FrameBufferView.ih"

namespace ospray {

namespace api {
struct ISPCDevice;
}

struct OSPRAY_SDK_INTERFACE FrameOp : public FrameOpInterface
{
  FrameOp(api::ISPCDevice &device) : device(device) {}
  ~FrameOp() override = default;

 protected:
  api::ISPCDevice &device;
};

struct OSPRAY_SDK_INTERFACE LiveFrameOp
    : public AddStructShared<LiveFrameOpInterface, ispc::FrameBufferView>
{
  LiveFrameOp(api::ISPCDevice &device, FrameBufferView &fbView);
  ~LiveFrameOp() override = default;

 protected:
  api::ISPCDevice &device;
};

} // namespace ospray
