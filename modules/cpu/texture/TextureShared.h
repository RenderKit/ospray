// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *Texture_get;
typedef void *Texture_getN;
#else
struct Texture;
struct DifferentialGeometry;
typedef varying vec4f (*Texture_get)(
    const Texture *uniform self, const varying DifferentialGeometry &dg);
typedef varying vec3f (*Texture_getN)(
    const Texture *uniform self, const varying DifferentialGeometry &dg);
#endif

enum TextureType
{
  TEXTURE_TYPE_2D,
#ifdef OSPRAY_ENABLE_VOLUMES
  TEXTURE_TYPE_VOLUME,
#endif
  TEXTURE_TYPE_UNKNOWN,
};

struct Texture
{
  TextureType type;
  Texture_get get;
  Texture_getN getNormal;
  bool hasAlpha; // 4 channel texture?

#ifdef __cplusplus
  Texture(bool hasAlpha = false)
      : type(TEXTURE_TYPE_UNKNOWN),
        get(nullptr),
        getNormal(nullptr),
        hasAlpha(hasAlpha)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
