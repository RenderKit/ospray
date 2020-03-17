// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../ImageOp.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp
{
  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;
};

struct OSPRAY_SDK_INTERFACE LiveDebugFrameOp : public LiveFrameOp
{
  LiveDebugFrameOp(FrameBufferView &fbView);
  void process(const Camera *) override;
};

} // namespace ospray
