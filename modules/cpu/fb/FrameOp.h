// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "common/ISPCRTBuffers.h"
#include "fb/FrameBufferView.h"
#include "fb/ImageOp.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE FrameOp : public FrameOpInterface
{
  FrameOp(api::ISPCDevice &device) : device(device) {}
  ~FrameOp() override = default;

 protected:
  api::ISPCDevice &device;
};

struct OSPRAY_SDK_INTERFACE LiveFrameOp : public LiveFrameOpInterface
{
  LiveFrameOp(api::ISPCDevice &device, FrameBufferView &fbView);
  ~LiveFrameOp() override = default;

 protected:
  inline const FrameBufferView &getFBView()
  {
    return *fbViewSh.data();
  }

  api::ISPCDevice &device;

 private:
  BufferShared<FrameBufferView> fbViewSh;
};

} // namespace ospray
