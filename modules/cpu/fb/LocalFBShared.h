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
  vec4f *colorBuffer;
  vec4f *varianceBuffer; // accumulates every other sample, for variance
                         // estimation / stopping
  float *depthBuffer;
  vec3f *normalBuffer;
  vec3f *albedoBuffer;
  vec2i numRenderTasks;
  uint32 *primitiveIDBuffer;
  uint32 *objectIDBuffer;
  uint32 *instanceIDBuffer;

#ifdef __cplusplus
  LocalFB()
      : colorBuffer(nullptr),
        varianceBuffer(nullptr),
        depthBuffer(nullptr),
        normalBuffer(nullptr),
        albedoBuffer(nullptr),
        numRenderTasks(0),
        primitiveIDBuffer(nullptr),
        objectIDBuffer(nullptr),
        instanceIDBuffer(nullptr)
  {
    super.type = FRAMEBUFFER_TYPE_LOCAL;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
