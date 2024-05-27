// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.ih"
#include "RenderTaskDesc.ih"
#include "Tile.ih"
#include "render/ScreenSample.ih"
// c++ shared
#include "LocalFBShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void LocalFB_accumulateSample(FrameBuffer *uniform _fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &)
{
  LocalFB *uniform fb = (LocalFB * uniform) _fb;
  const int pixelID =
      screenSample.sampleID.x + fb->super.size.x * screenSample.sampleID.y;

  // Calculate accumulation scale
  const uniform int accumID = fb->super.frameID;
  const uniform float accScale = rcpf(accumID + 1);

  // Accumulate color
  const vec4f colorSample = make_vec4f(screenSample.rgb, screenSample.alpha);
  if (fb->colorBuffer)
    FRAMEBUFFER_ACCUMULATE_VALUE(
        fb->colorBuffer[pixelID], colorSample, accScale);

  // Accumulate color in variance buffer
  if (fb->super.accumulateVariance) {
    const uniform float accVarScale = rcpf(accumID / 2 + 1);
    FRAMEBUFFER_ACCUMULATE_VALUE(
        fb->varianceBuffer[pixelID], colorSample, accVarScale);
  }

  // Accumulate depth buffer
  if (fb->depthBuffer) {
    float depth = screenSample.z;
    if (accumID > 0)
      depth = min(fb->depthBuffer[pixelID], screenSample.z);
    fb->depthBuffer[pixelID] = depth;
  }

  // Accumulate normal buffer
  if (fb->normalBuffer)
    FRAMEBUFFER_ACCUMULATE_VALUE(
        fb->normalBuffer[pixelID], screenSample.normal, accScale);

  // Accumulate albedo buffer
  if (fb->albedoBuffer)
    FRAMEBUFFER_ACCUMULATE_VALUE(
        fb->albedoBuffer[pixelID], screenSample.albedo, accScale);

  if (fb->primitiveIDBuffer) {
    if (accumID == 0)
      fb->primitiveIDBuffer[pixelID] = screenSample.primID;
  }

  if (fb->objectIDBuffer) {
    if (accumID == 0)
      fb->objectIDBuffer[pixelID] = screenSample.geomID;
  }

  if (fb->instanceIDBuffer) {
    if (accumID == 0)
      fb->instanceIDBuffer[pixelID] = screenSample.instID;
  }

#ifndef OSPRAY_TARGET_SYCL
  FrameBuffer_updateProgress(&fb->super, popcnt(lanemask()));
#endif
}

SYCL_EXTERNAL uniform RenderTaskDesc LocalFB_getRenderTaskDesc(
    FrameBuffer *uniform _fb, const uniform uint32 taskID)
{
  LocalFB *uniform fb = (LocalFB * uniform) _fb;
  uniform RenderTaskDesc desc;
  desc.taskID = taskID;
  desc.error = 0.f;

  uniform vec2i renderTaskSize = fb->super.renderTaskSize;
  uniform vec2i taskStart =
      make_vec2i(taskID % fb->numRenderTasks.x, taskID / fb->numRenderTasks.x);
  desc.region.lower = taskStart * renderTaskSize;
  desc.region.upper = min(desc.region.lower + renderTaskSize, fb->super.size);
  return desc;
}

SYCL_EXTERNAL void LocalFB_completeTask(
    FrameBuffer *uniform, const uniform RenderTaskDesc &)
{
  // Do nothing
}

// TODO: Unify all writeTile functions below into a single one
export SYCL_EXTERNAL void LocalFrameBuffer_writeColorTile(
    void *uniform _fb, const void *uniform _tile)
{
  Tile *uniform tile = (Tile * uniform) _tile;
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  uniform vec4f *uniform color = (uniform vec4f * uniform) fb->colorBuffer;

  color += (uniform uint64)tile->region.lower.y * fb->super.size.x;
  VaryingTile *uniform varyTile = (VaryingTile * uniform) tile;
  for (uniform uint32 iiy = tile->region.lower.y;
       iiy < (uniform uint32)tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);
    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < (uniform uint32)tile->region.upper.x;
         iix += programCount, chunkID++) {
      color[iix] = make_vec4f(varyTile->r[chunkID],
          varyTile->g[chunkID],
          varyTile->b[chunkID],
          varyTile->a[chunkID]);
    }
    color += fb->super.size.x;
  }
}

// Write the input tile into the row-major framebuffer's depth buffer
export SYCL_EXTERNAL void LocalFrameBuffer_writeDepthTile(
    void *uniform _fb, const void *uniform _tile)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;

  VaryingTile *uniform varyTile = (VaryingTile * uniform) tile;
  uniform float *uniform depth = (uniform float *uniform)fb->depthBuffer;
  depth += (uniform uint64)tile->region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile->region.lower.y;
       iiy < (uniform uint32)tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < (uniform uint32)tile->region.upper.x;
         iix += programCount, chunkID++) {
      depth[iix] = varyTile->z[chunkID];
    }
    depth += fb->super.size.x;
  }
}

// "accumulate" first frame only and write into buffer
export SYCL_EXTERNAL void LocalFrameBuffer_writeIDTile(void *uniform _fb,
    const void *uniform _tile,
    uniform uint32 *uniform dst,
    const void *uniform src)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;
  uniform uint32 *uniform buffer =
      dst; //(uniform uint32 *uniform)fb->primitiveIDBuffer;
  buffer += (uniform uint64)tile->region.lower.y * fb->super.size.x;
  const void *uniform upid = src; //(const void* uniform)tile->pid;
  const varying int32 *uniform pid = (const varying int32 *uniform)upid;

  for (uniform uint32 iiy = tile->region.lower.y;
       iiy < (uniform uint32)tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < (uniform uint32)tile->region.upper.x;
         iix += programCount, chunkID++) {
      if (tile->accumID == 0) {
        varying uint32 val = -1;
        unmasked
        {
          val = pid[chunkID];
        }
        buffer[iix] = val;
      }
    }
    buffer += fb->super.size.x;
  }
}

// Write a Tile's auxiliary buffer into the framebuffer's row major storage
export SYCL_EXTERNAL void LocalFrameBuffer_writeAuxTile(void *uniform _fb,
    const void *uniform _tile,
    void *uniform _aux,
    const void *uniform _ax,
    const void *uniform _ay,
    const void *uniform _az)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const Tile *uniform tile = (const Tile *uniform)_tile;
  uniform vec3f *uniform aux = (uniform vec3f * uniform) _aux;
  const varying float *uniform ax = (const varying float *uniform)_ax;
  const varying float *uniform ay = (const varying float *uniform)_ay;
  const varying float *uniform az = (const varying float *uniform)_az;

  aux += (uniform uint64)tile->region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile->region.lower.y;
       iiy < (uniform uint32)tile->region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile->region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile->region.lower.x + programIndex;
         iix < (uint32)tile->region.upper.x;
         iix += programCount, chunkID++) {
      aux[iix] = make_vec3f(ax[chunkID], ay[chunkID], az[chunkID]);
    }
    aux += fb->super.size.x;
  }
}

export void *uniform LocalFrameBuffer_accumulateSample_addr()
{
  return (void *uniform)LocalFB_accumulateSample;
}

export void *uniform LocalFrameBuffer_getRenderTaskDesc_addr()
{
  return (void *uniform)LocalFB_getRenderTaskDesc;
}

export void *uniform LocalFrameBuffer_completeTask_addr()
{
  return (void *uniform)LocalFB_completeTask;
}

OSPRAY_END_ISPC_NAMESPACE
