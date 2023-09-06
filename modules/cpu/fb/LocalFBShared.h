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
  vec4f *varianceBuffer; // accumulates every other sample, for variance estimation / stopping
  vec3f *normalBuffer;
  vec3f *albedoBuffer;
  int32 *taskAccumID; // holds accumID per tile, for adaptive accumulation
  float *taskRegionError;
  uint32 varianceAccumCount;
  uint32 accumulateVariance; // could be boolean but ISPC hates them
  vec2i numRenderTasks;
  uint32 *primitiveIDBuffer;
  uint32 *objectIDBuffer;
  uint32 *instanceIDBuffer;

#ifdef __cplusplus
  LocalFB()
      : colorBuffer(nullptr),
        depthBuffer(nullptr),
        accumBuffer(nullptr),
        varianceBuffer(nullptr),
        normalBuffer(nullptr),
        albedoBuffer(nullptr),
        taskAccumID(nullptr),
        taskRegionError(nullptr),
        varianceAccumCount(0),
        accumulateVariance(0),
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
