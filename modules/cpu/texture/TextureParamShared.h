// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TextureShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

enum TransformFlags
{
  TRANSFORM_FLAG_NONE = 0x0,
  TRANSFORM_FLAG_2D = 0x1,
  TRANSFORM_FLAG_3D = 0x2
};

struct TextureParam
{
  Texture *ptr;
  TransformFlags transformFlags;
  affine2f xform2f;
  affine3f xform3f;

#ifdef __cplusplus
  TextureParam()
      : ptr(nullptr),
        transformFlags(TRANSFORM_FLAG_NONE),
        xform2f(one),
        xform3f(one)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
