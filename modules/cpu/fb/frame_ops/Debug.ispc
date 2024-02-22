// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOpKernelLaunch.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "fb/FrameBufferView.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void Debug_kernel(const FrameBufferView *uniform self, const vec2ui id)
{
  const uint32 i = id.y * self->viewDims.x + id.x;
  vec4f v = self->colorBufferInput[i];
  v.x = 1.f;
  self->colorBufferOutput[i] = v;
}

DEFINE_KERNEL_LAUNCHER(Debug_kernel);

OSPRAY_END_ISPC_NAMESPACE
