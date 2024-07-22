// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TextureShared.h"
#include "ospray/OSPEnums.h"

#define MAX_MIPMAP_LEVEL 32
#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Texture2D
{
  Texture super;

  vec2i size;

  void *data[MAX_MIPMAP_LEVEL];
  int maxLevel;

  OSPTextureFormat format;
  OSPTextureFilter filter;
  vec2ui wrapMode;

#ifdef __cplusplus
  Texture2D()
      : size(0),
        data{nullptr},
        maxLevel(0),
        format(OSP_TEXTURE_FORMAT_INVALID),
        filter(OSP_TEXTURE_FILTER_LINEAR),
        wrapMode(vec2ui(OSP_TEXTURE_WRAP_REPEAT))
  {
    super.type = TEXTURE_TYPE_2D;
  }
  void set(const vec2i &aSize,
      void **aData,
      int maxLevel,
      OSPTextureFormat format,
      OSPTextureFilter flags,
      const vec2ui &wrapMode);
};
} // namespace ispc
#else
};
#endif // __cplusplus
