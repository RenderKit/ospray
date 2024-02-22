// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOpKernelLaunch.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "BlurShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void BlurHorizontal_kernel(
    const FrameBufferView *uniform fbvSelf, const vec2ui id)
{
  const LiveBlur *uniform self = (const LiveBlur *uniform)fbvSelf;
  uniform const vec2ui &dims = fbvSelf->viewDims;
  const vec4f *uniform inputBuffer = fbvSelf->colorBufferInput;
  vec4f v = make_vec4f(0.f);
  for (int32 i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++) {
    const uint32 i0 = id.x + i;
    const uint32 absi = abs(i);
    if (i0 < dims.x) {
      v = v + inputBuffer[id.y * dims.x + i0] * self->weights[absi];
    }
  }
  self->scratchBuffer[id.y * dims.x + id.x] = v;
}

inline void BlurVertical_kernel(
    const FrameBufferView *uniform fbvSelf, const vec2ui id)
{
  const LiveBlur *uniform self = (const LiveBlur *uniform)fbvSelf;
  uniform const vec2ui &dims = fbvSelf->viewDims;
  const vec4f *uniform inputBuffer = self->scratchBuffer;
  vec4f v = make_vec4f(0.f);
  for (int32 i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++) {
    const uint32 i0 = id.y + i;
    const uint32 absi = abs(i);
    if (i0 < dims.y) {
      v = v + inputBuffer[i0 * dims.x + id.x] * self->weights[absi];
    }
  }
  fbvSelf->colorBufferOutput[id.y * dims.x + id.x] = v;
}

DEFINE_KERNEL_LAUNCHER(BlurHorizontal_kernel);
DEFINE_KERNEL_LAUNCHER(BlurVertical_kernel);

OSPRAY_END_ISPC_NAMESPACE
