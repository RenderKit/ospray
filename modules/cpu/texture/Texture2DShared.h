// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TextureShared.h"

#ifdef __cplusplus
#include "Mapping.h"
#else
#include "Mapping.ih"
#endif

#ifdef __cplusplus
using namespace rkcommon::math;
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

  EnsightTex1dMappingData map1d;  //private mapping alg. data for EnSight's colorby using raw var

#ifdef __cplusplus
  Texture2D() : size(0), sizef(0.f), halfTexel(0.f), data(nullptr), map1d() {}
  void Set(const vec2i &aSize,
      void *aData,
      const EnsightTex1dMappingData &map1d,
      OSPTextureFormat type,
      OSPTextureFilter flags);
};
} // namespace ispc
#else
};
#endif // __cplusplus
