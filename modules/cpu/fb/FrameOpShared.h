// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#include "fb/FrameBufferView.h"

struct LiveFrameOp
{
  FrameBufferView fbView;

#ifdef __cplusplus
  LiveFrameOp() : fbView(vec2ui(0, 0), nullptr, nullptr, nullptr, nullptr) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
