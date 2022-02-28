// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospray/OSPEnums.h"

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

struct FrameBuffer
{
  vec2i size; // size (width x height) of frame buffer, in pixels
  vec2f rcpSize; // one over size (precomputed)

  // This marks the global number of frames that have been rendered since
  // the last ospFramebufferClear() has been called.
  int32 frameID;

  OSPFrameBufferFormat colorBufferFormat;

#ifdef __cplusplus
  FrameBuffer()
      : size(0), rcpSize(0.f), frameID(-1), colorBufferFormat(OSP_FB_NONE)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
