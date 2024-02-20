// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/FrameBufferView.ih"

#ifdef __cplusplus
namespace ispc {
#endif

struct LiveVariance
{
  FrameBufferView super;
  vec2ui rtSize;
  const vec4f *varianceBuffer;
  float *taskVarianceBuffer;
};
#ifdef __cplusplus
}
#endif
