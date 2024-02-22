// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/FrameBufferView.ih"
#include "ospray/OSPEnums.h"

#ifdef __cplusplus
namespace ispc {
#endif

struct LiveColorConversion
{
  FrameBufferView super;

  // Target color format
  OSPFrameBufferFormat targetColorFormat;

  // Converted color buffer
  void *convBuffer;
};
#ifdef __cplusplus
}
#endif
