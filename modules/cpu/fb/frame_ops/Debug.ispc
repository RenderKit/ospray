// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DeviceRT.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "fb/FrameBufferView.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void Debug_kernel(
    const vec2ui itemIndex, const FrameBufferView *uniform self)
{
  const uint32 i = itemIndex.y * self->viewDims.x + itemIndex.x;
  vec4f v = self->colorBufferInput[i];
  v.x = 1.f;
  self->colorBufferOutput[i] = v;
}

DEFINE_FRAMEOP_KERNEL_LAUNCHER(Debug_kernel);

OSPRAY_END_ISPC_NAMESPACE
