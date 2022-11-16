// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#if defined(__cplusplus)
namespace ispc {
#endif // __cplusplus

// TODO: We could remove this child struct
struct DistributedRenderer
{
  Renderer super;

#ifdef __cplusplus
  DistributedRenderer() {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
