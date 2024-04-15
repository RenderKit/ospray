// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "fb/FrameOp.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp
{
  DebugFrameOp(devicert::Device &device);

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveDebugFrameOp : public LiveFrameOp
{
  LiveDebugFrameOp(devicert::Device &device, FrameBufferView &fbView);
  devicert::AsyncEvent process() override;
};

} // namespace ospray
