// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/FrameOpShared.h"

#ifdef __cplusplus
namespace ispc {
#endif

#define BLUR_RADIUS 4
struct LiveBlur
{
  LiveFrameOp super;

  float weights[BLUR_RADIUS + 1];
};
#ifdef __cplusplus
}
#endif
