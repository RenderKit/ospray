// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameOpKernelLaunch.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "fb/FrameOpShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void Debug_kernel(const LiveFrameOp *uniform self,
    const vec2ui id,
    const vec4f *uniform inputBuffer,
    vec4f *uniform outputBuffer)
{
  uniform const vec2ui &dims = self->fbView.viewDims;
  const uint32 i = id.y * dims.x + id.x;
  vec4f v = inputBuffer[i];
  v.x = 1.f;
  outputBuffer[i] = v;
}

DEFINE_KERNEL_LAUNCHER(Debug_kernel);

OSPRAY_END_ISPC_NAMESPACE
