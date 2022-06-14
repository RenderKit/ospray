// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TextureShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Texture2D
{
  Texture super;

  vec2i size;
  vec2f sizef; // size, as floats; slightly smaller than 'size' to avoid range
               // checks
  vec2f halfTexel; // 0.5/size, needed for bilinear filtering and clamp-to-edge
  void *data;

#ifdef __cplusplus
  Texture2D() : size(0), sizef(0.f), halfTexel(0.f), data(nullptr) {}
  void set(const vec2i &aSize,
      void *aData,
      OSPTextureFormat type,
      OSPTextureFilter flags);
};
} // namespace ispc
#else
};
#endif // __cplusplus
