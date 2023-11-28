// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/ISPCRTBuffers.h"
#include "fb/FrameOp.h"
// ispc shared
#include "VarianceShared.h"

namespace ospray {

// The variance frame op calculates per-RenderTask variance
struct OSPRAY_SDK_INTERFACE LiveVarianceFrameOp
    : public AddStructShared<LiveFrameOp, ispc::LiveVariance>
{
  LiveVarianceFrameOp(api::ISPCDevice &device,
      FrameBufferView &fbView,
      const vec4f *varianceBuffer);

  void process(void *) override;
  void restart();

  bool validError() const;
  float getError(const uint32_t id) const;
  float getMaxError() const;

 private:
  BufferShared<float> taskVariance;
};

inline void LiveVarianceFrameOp::restart()
{
  getSh()->firstRun = true;
}

inline bool LiveVarianceFrameOp::validError() const
{
  return getSh()->firstRun == false;
}

inline float LiveVarianceFrameOp::getError(const uint32_t id) const
{
  return taskVariance[id];
}

} // namespace ospray
