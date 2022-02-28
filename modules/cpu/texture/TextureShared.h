// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
typedef void *Texture_get;
typedef void *Texture_getN;
#else
struct Texture;
struct DifferentialGeometry;
typedef varying vec4f (*Texture_get)(
    const Texture *uniform self, const varying DifferentialGeometry &dg);
typedef varying vec3f (*Texture_getN)(
    const Texture *uniform self, const varying DifferentialGeometry &dg);
#endif // __cplusplus

struct Texture
{
  Texture_get get;
  Texture_getN getNormal;
  bool hasAlpha; // 4 channel texture?

#ifdef __cplusplus
  Texture(bool hasAlpha = false)
      : get(nullptr), getNormal(nullptr), hasAlpha(hasAlpha)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
