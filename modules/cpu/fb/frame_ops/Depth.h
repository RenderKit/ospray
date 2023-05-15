// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOp.h"

namespace ospray {

//! Depth frameop replaces the color data with a normalized depth buffer img
struct OSPRAY_SDK_INTERFACE DepthFrameOp : public FrameOp
{
  DepthFrameOp(api::Device &device)
      : FrameOp(static_cast<api::ISPCDevice &>(device))
  {}

  std::unique_ptr<LiveFrameOpInterface> attach(
      FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveDepthFrameOp : public LiveFrameOp
{
  LiveDepthFrameOp(api::ISPCDevice &device, FrameBufferView &fbView);

  void process(void *, const Camera *) override;
};

} // namespace ospray
