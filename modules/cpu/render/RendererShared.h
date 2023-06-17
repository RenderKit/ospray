// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Texture2D;
struct Material;
struct PixelFilter;
struct MathConstants;

struct Renderer
{
  int32 spp;
  vec4f bgColor; // background color and alpha
  Texture2D *backplate;
  Texture2D *maxDepthTexture; // optional maximum depth texture used for early
                              // ray termination
  uint32 maxDepth;
  float minContribution;

  int32 numMaterials;
  Material **material;

  PixelFilter *pixelFilter;
  MathConstants *mathConstants;

#ifdef __cplusplus
  Renderer()
      : spp(1),
        bgColor(0.f),
        backplate(nullptr),
        maxDepthTexture(nullptr),
        maxDepth(20),
        minContribution(0.001f),
        numMaterials(0),
        material(nullptr),
        pixelFilter(nullptr),
        mathConstants(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
