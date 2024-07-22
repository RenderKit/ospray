// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DeviceRT.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "BlurShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void BlurHorizontal_kernel(
    const vec2ui itemIndex, const FrameBufferView *uniform fbvSelf)
{
  const LiveBlur *uniform self = (const LiveBlur *uniform)fbvSelf;
  uniform const vec2ui &itemDims = fbvSelf->viewDims;
  const vec4f *uniform inputBuffer = fbvSelf->colorBufferInput;
  vec4f v = make_vec4f(0.f);
  for (int32 i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++) {
    const uint32 i0 = itemIndex.x + i;
    const uint32 absi = abs(i);
    if (i0 < itemDims.x) {
      v = v + inputBuffer[itemIndex.y * itemDims.x + i0] * self->weights[absi];
    }
  }
  self->scratchBuffer[itemIndex.y * itemDims.x + itemIndex.x] = v;
}

inline void BlurVertical_kernel(
    const vec2ui itemIndex, const FrameBufferView *uniform fbvSelf)
{
  const LiveBlur *uniform self = (const LiveBlur *uniform)fbvSelf;
  uniform const vec2ui &itemDims = fbvSelf->viewDims;
  const vec4f *uniform inputBuffer = self->scratchBuffer;
  vec4f v = make_vec4f(0.f);
  for (int32 i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++) {
    const uint32 i0 = itemIndex.y + i;
    const uint32 absi = abs(i);
    if (i0 < itemDims.y) {
      v = v + inputBuffer[i0 * itemDims.x + itemIndex.x] * self->weights[absi];
    }
  }
  fbvSelf->colorBufferOutput[itemIndex.y * itemDims.x + itemIndex.x] = v;
}

DEFINE_FRAMEOP_KERNEL_LAUNCHER(BlurHorizontal_kernel);
DEFINE_FRAMEOP_KERNEL_LAUNCHER(BlurVertical_kernel);

OSPRAY_END_ISPC_NAMESPACE
