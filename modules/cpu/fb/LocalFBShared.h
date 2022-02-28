// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FrameBufferShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// a Local FrameBuffer that stores all pixel values (color, depth,
// accum) in a plain 2D array of pixels (one array per component)
struct LocalFB
{
  FrameBuffer super; // superclass that we inherit from
  void *colorBuffer;
  float *depthBuffer;
  vec4f *accumBuffer;
  vec4f *varianceBuffer; // accumulates every other sample, for variance
                         // estimation / stopping
  vec3f *normalBuffer;
  vec3f *albedoBuffer;
  int32 *tileAccumID; // holds accumID per tile, for adaptive accumulation
  vec2i numTiles;

#ifdef __cplusplus
  LocalFB()
      : colorBuffer(nullptr),
        depthBuffer(nullptr),
        accumBuffer(nullptr),
        varianceBuffer(nullptr),
        normalBuffer(nullptr),
        albedoBuffer(nullptr),
        tileAccumID(nullptr),
        numTiles(0)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
