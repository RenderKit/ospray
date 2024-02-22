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
  float getAvgError(const float errorThreshold) const;

 private:
  BufferShared<float> taskVariance;
  bool firstRun{true};
};

inline void LiveVarianceFrameOp::restart()
{
  firstRun = true;
}

inline bool LiveVarianceFrameOp::validError() const
{
  return firstRun == false;
}

inline float LiveVarianceFrameOp::getError(const uint32_t id) const
{
  return taskVariance[id];
}

} // namespace ospray
