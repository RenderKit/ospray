// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/PixelOpShared.h"
#include "ospray/OSPEnums.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_SYCL)
typedef void *FrameBuffer_accumulateSampleFct;
typedef void *FrameBuffer_getRenderTaskDescFct;
typedef void *FrameBuffer_completeTaskFct;
#else
struct FrameBuffer;
struct ScreenSample;
struct RenderTaskDesc;

typedef void (*FrameBuffer_accumulateSampleFct)(FrameBuffer *uniform fb,
    const varying ScreenSample &sample,
    uniform RenderTaskDesc &taskDesc);

typedef uniform RenderTaskDesc (*FrameBuffer_getRenderTaskDescFct)(
    FrameBuffer *uniform fb, const uniform uint32 taskID);

typedef void (*FrameBuffer_completeTaskFct)(
    FrameBuffer *uniform fb, const uniform RenderTaskDesc &taskDesc);
#endif

enum FrameBufferType
{
  FRAMEBUFFER_TYPE_LOCAL,
  FRAMEBUFFER_TYPE_SPARSE,
  FRAMEBUFFER_TYPE_UNKNOWN,
};

/* The ISPC-side FrameBuffer allows tasks to write directly to the framebuffer
 * memory from ISPC. Given the set of task IDs to be rendered, a renderer must:
 *
 * 1. Get the RenderTaskDesc by calling getRenderTaskDesc and render the region
 * of pixels specified in the task description.
 *
 * 2. Write the result of rendering each pixel to the framebuffer by calling
 * accumulateSample
 *
 * 3. Update task-level values (accum ID and error) by calling completeTask
 */
struct FrameBuffer
{
  FrameBufferType type;
  /* Get the task description for a given render task ID. The task description
   * stores the region that should be rendered for the task and its accumID
   */
  FrameBuffer_getRenderTaskDescFct getRenderTaskDesc;

  /* Accumulate samples taken for this render task into the framebuffer.
   * Task error will also be computed and accumulated on the render task,
   * to handle cases where there are more pixels in a task than the SIMD width.
   */
  FrameBuffer_accumulateSampleFct accumulateSample;

  /* Perform final task updates for the given task, updating its accum ID (if
   * accumulation buffering is enabled) and its error if variance termination is
   * enabled
   */
  FrameBuffer_completeTaskFct completeTask;

  // The size of the framebuffer, in pixels
  vec2i size;
  // 1/size (precomputed)
  vec2f rcpSize;
  // The default size of each each render task, in pixels
  vec2i renderTaskSize;

  // Not used on GPU to avoid USM thrashing
  int32 frameID;

  // The channels stored in the framebuffer
  uint32 channels;

  OSPFrameBufferFormat colorBufferFormat;

  LivePixelOp **pixelOps;
  uint32 numPixelOps;

  // If the frame has been cancelled or not. Note: we don't share bools between
  // ISPC and C++ as the true value representation may differ (as it does with
  // gcc)
  uint32 cancelRender;

  // The number of pixels rendered this frame, for tracking rendering progress
  // Not used on GPU to avoid USM thrashing
  uint32 numPixelsRendered;

#ifdef __cplusplus
  FrameBuffer()
      : type(FRAMEBUFFER_TYPE_UNKNOWN),
        getRenderTaskDesc(nullptr),
        accumulateSample(nullptr),
        completeTask(nullptr),
        size(0),
        rcpSize(0.f),
        renderTaskSize(4),
        frameID(-1),
        channels(0),
        colorBufferFormat(OSP_FB_NONE),
        pixelOps(nullptr),
        numPixelOps(0),
        cancelRender(0),
        numPixelsRendered(0)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
