// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Variance.h"
#include "fb/FrameBufferView.h"

DECLARE_FRAMEOP_KERNEL_LAUNCHER(Variance_kernelLauncher);

namespace ospray {

LiveVarianceFrameOp::LiveVarianceFrameOp(devicert::Device &device,
    FrameBufferView &fbView,
    const vec4f *varianceBuffer)
    : AddStructShared(device, device, fbView),
      taskVariance(device, fbView.viewDims.product())
{
  // Prepare kernel constant variables
  getSh()->rtSize = divRoundUp(fbView.fbDims, fbView.viewDims);
  getSh()->varianceBuffer = varianceBuffer;
  getSh()->taskVarianceBuffer = taskVariance.data();
}

devicert::AsyncEvent LiveVarianceFrameOp::process()
{
  // Run kernel
  const vec2ui &itemDims = getSh()->super.viewDims;
  firstRun = false;
  return device.launchFrameOpKernel(
      itemDims, ispc::Variance_kernelLauncher, &getSh()->super);
}

float LiveVarianceFrameOp::getAvgError(const float errorThreshold) const
{
  float maxErr = 0.f;
  float sumActErr = 0.f;
  int activeTasks = 0;
  std::for_each(
      taskVariance.cbegin(), taskVariance.cend(), [&](const float &err) {
        maxErr = std::max(maxErr, err);
        if (err >= errorThreshold) {
          sumActErr += err;
          activeTasks++;
        }
      });
  return activeTasks ? sumActErr / activeTasks : maxErr;
}

} // namespace ospray
