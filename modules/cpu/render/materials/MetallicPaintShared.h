// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct MetallicPaint
{
  Material super;

  vec3f baseColor;
  TextureParam baseColorMap;
  float flakeAmount;
  vec3f flakeColor;
  float flakeSpread;
  float eta;

#ifdef __cplusplus
  MetallicPaint()
      : baseColor(.25f),
        flakeAmount(.5f),
        flakeColor(flakeAmount * vec3f(RGB_AL_COLOR)),
        flakeSpread(.5),
        eta(1.f / 1.6f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
