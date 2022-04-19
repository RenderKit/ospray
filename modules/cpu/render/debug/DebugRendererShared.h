// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DebugRendererType.h"
#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct DebugRenderer
{
  Renderer super;
  DebugRendererType type;

#ifdef __cplusplus
  DebugRenderer() : type(TEST_FRAME)
  {
    super.type = RENDERER_TYPE_DEBUG;
  }
#endif
};
#ifdef __cplusplus
}
#endif
