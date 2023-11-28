// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOpKernelLaunch.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "VarianceShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void Variance_kernel(
    const FrameBufferView *uniform fbvSelf, const vec2ui id)
{
  uniform const vec2ui &rtDims = fbvSelf->viewDims;
  uniform const vec2ui &fbDims = fbvSelf->fbDims;
  const vec4f *uniform colorBuffer = fbvSelf->colorBufferInput;

  const LiveVariance *uniform self = (const LiveVariance *uniform)fbvSelf;
  uniform const vec2ui rtSize = self->rtSize;
  const vec4f *uniform varianceBuffer = self->varianceBuffer;
  const vec2ui rtLower = id * rtSize;

  // Iterate over all pixels within single render task and calculate maximum
  // variance over collected samples
  float v = 0.f;
  for (uniform uint32 rtY = 0; rtY < rtSize.y; rtY++) {
    // Skip if out of range
    const uint32 fbY = rtLower.y + rtY;
    if (fbY >= fbDims.y)
      continue;

    for (uniform uint32 rtX = 0; rtX < rtSize.x; rtX++) {
      // Skip if out of range
      const uint32 fbX = rtLower.x + rtX;
      if (fbX >= fbDims.x)
        continue;

      const uint32 linearId = fbY * fbDims.x + fbX;
      const vec4f color = colorBuffer[linearId];
      const vec4f variance = varianceBuffer[linearId];

      const float den2 = reduce_add(make_vec3f(color))
          + (1.f - color.w); // invert alpha (bright alpha is more important)
      if (den2 > 0.f) {
        const vec4f diff = absf(color - variance);
        v = max(v, reduce_add(diff) * rsqrtf(den2));
      }
    }
  }

  // Average maximum variance between two subsequent accumulations to have more
  // uniform spatial distribution
  const uint32 lId = id.y * rtDims.x + id.x;
  if (!self->firstRun)
    v = lerp(.25f, self->taskVarianceBuffer[lId], v);

  // Output final variance
  self->taskVarianceBuffer[lId] = v;
}

DEFINE_KERNEL_LAUNCHER(Variance_kernel);

OSPRAY_END_ISPC_NAMESPACE
