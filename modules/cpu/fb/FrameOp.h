// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/StructShared.h"
#include "fb/ImageOp.h"
// ispc shared
#include "fb/FrameBufferView.ih"

namespace ospray {

struct OSPRAY_SDK_INTERFACE FrameOp : public FrameOpInterface
{
  FrameOp(devicert::Device &device) : device(device) {}
  ~FrameOp() override = default;

 protected:
  devicert::Device &device;
};

struct OSPRAY_SDK_INTERFACE LiveFrameOp
    : public AddStructShared<LiveFrameOpInterface, ispc::FrameBufferView>
{
  LiveFrameOp(devicert::Device &device, FrameBufferView &fbView);
  ~LiveFrameOp() override = default;

 protected:
  devicert::Device &device;
};

} // namespace ospray
