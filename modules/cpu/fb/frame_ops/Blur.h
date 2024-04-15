// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOp.h"
// ispc shared
#include "BlurShared.h"

namespace ospray {

// The blur frame op is a test which applies a Gaussian blur to the frame
struct OSPRAY_SDK_INTERFACE BlurFrameOp : public FrameOp
{
  BlurFrameOp(devicert::Device &device);

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveBlurFrameOp
    : public AddStructShared<LiveFrameOp, ispc::LiveBlur>
{
  LiveBlurFrameOp(devicert::Device &device, FrameBufferView &fbView);
  devicert::AsyncEvent process() override;

 private:
  devicert::BufferDevice<vec4f> scratch;
};

} // namespace ospray
