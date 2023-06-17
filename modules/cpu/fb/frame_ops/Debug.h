// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "fb/FrameOp.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp
{
  DebugFrameOp(api::Device &device)
      : FrameOp(static_cast<api::ISPCDevice &>(device))
  {}

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveDebugFrameOp : public LiveFrameOp
{
  LiveDebugFrameOp(api::ISPCDevice &device, FrameBufferView &fbView);
  void process(void *waitEvent, const Camera *) override;
};

} // namespace ospray
