// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FrameBufferShared.h"
#include "embree4/rtcore_config.h"

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

  // accumulates every other sample, for variance estimation / stopping
  vec4f *varianceBuffer;
  vec3f *normalBuffer;
  vec3f *albedoBuffer;
  // holds accumID per tile, for adaptive accumulation
  int32 *taskAccumID;
  float *taskRegionError;
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
        numRenderTasks(0),
        primitiveIDBuffer(nullptr),
        objectIDBuffer(nullptr),
        instanceIDBuffer(nullptr)
  {
    super.type = FRAMEBUFFER_TYPE_LOCAL;
  }
};
#ifdef OSPRAY_TARGET_SYCL
void LocalFrameBuffer_writeTile_RGBA8(void *_fb, const void *_tile);
void LocalFrameBuffer_writeTile_SRGBA(void *_fb, const void *_tile);
void LocalFrameBuffer_writeTile_RGBA32F(void *_fb, const void *_tile);
void LocalFrameBuffer_writeDepthTile(void *_fb, const void *uniform _tile);
void LocalFrameBuffer_writeAuxTile(void *_fb,
    const void *_tile,
    vec3f *aux,
    const void *_ax,
    const void *_ay,
    const void *_az);
void LocalFrameBuffer_writeIDTile(void *uniform _fb,
    const void *uniform _tile,
    uniform uint32 *uniform dst,
    const void *uniform src);

SYCL_EXTERNAL void LocalFB_accumulateSample(FrameBuffer *uniform _fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &taskDesc);

SYCL_EXTERNAL uniform RenderTaskDesc LocalFB_getRenderTaskDesc(
    FrameBuffer *uniform _fb, const uniform uint32 taskID);

SYCL_EXTERNAL void LocalFB_completeTask(
    FrameBuffer *uniform _fb, const uniform RenderTaskDesc &taskDesc);
#endif
} // namespace ispc
#else
};
#endif // __cplusplus
