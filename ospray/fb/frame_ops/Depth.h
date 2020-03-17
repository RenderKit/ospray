// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../ImageOp.h"

namespace ospray {

//! Depth frameop replaces the color data with a normalized depth buffer img
struct OSPRAY_SDK_INTERFACE DepthFrameOp : public FrameOp
{
  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveDepthFrameOp : public LiveFrameOp
{
  LiveDepthFrameOp(FrameBufferView &fbView);

  void process(const Camera *) override;
};

} // namespace ospray
