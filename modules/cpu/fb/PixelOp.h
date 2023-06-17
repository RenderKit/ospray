// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "common/StructShared.h"
#include "fb/ImageOp.h"
// ispc shared
#include "PixelOpShared.h"

namespace ospray {

struct FrameBufferView;

struct OSPRAY_SDK_INTERFACE LivePixelOp
    : public AddStructShared<LiveImageOp, ispc::LivePixelOp>
{
  LivePixelOp(api::ISPCDevice &device);
  ~LivePixelOp() override = default;
};

struct OSPRAY_SDK_INTERFACE PixelOp : public ImageOp
{
  PixelOp(api::ISPCDevice &device) : device(device) {}
  ~PixelOp() override = default;

  virtual std::unique_ptr<LivePixelOp> attach() = 0;

 protected:
  api::ISPCDevice &device;
};

} // namespace ospray
