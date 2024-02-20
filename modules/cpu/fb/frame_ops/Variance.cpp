// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Variance.h"
#include "ISPCDevice.h"
#include "fb/FrameBufferView.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void Variance_kernelLauncher(const FrameBufferView *, void *, void *);
} // namespace ispc
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

LiveVarianceFrameOp::LiveVarianceFrameOp(api::ISPCDevice &device,
    FrameBufferView &fbView,
    const vec4f *varianceBuffer)
    : AddStructShared(device.getIspcrtContext(), device, fbView),
      taskVariance(device.getIspcrtContext(), fbView.viewDims.product())
{
  // Prepare kernel constant variables
  getSh()->rtSize = divRoundUp(fbView.fbDims, fbView.viewDims);
  getSh()->varianceBuffer = varianceBuffer;
  getSh()->taskVarianceBuffer = taskVariance.data();
}

void LiveVarianceFrameOp::process(void *waitEvent)
{
  // Run kernel
  void *cmdQueue = nullptr;
#ifdef OSPRAY_TARGET_SYCL
  cmdQueue = &device.getSyclQueue();
#endif
  ispc::Variance_kernelLauncher(&getSh()->super, cmdQueue, waitEvent);
  firstRun = false;
}

float LiveVarianceFrameOp::getAvgError(const float errorThreshold) const
{
  float maxErr = 0.f;
  float sumActErr = 0.f;
  int activeTasks = 0;
  std::for_each(taskVariance.cbegin(), taskVariance.cend(), [&](const float &err) {
    maxErr = std::max(maxErr, err);
    if (err >= errorThreshold) {
      sumActErr += err;
      activeTasks++;
    }
  });
  return activeTasks ? sumActErr / activeTasks : maxErr;
}

} // namespace ospray
