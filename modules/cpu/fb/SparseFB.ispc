// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SparseFB.ih"
#include "fb/FrameBuffer.ih"
#include "fb/RenderTaskDesc.ih"
#include "fb/Tile.ih"
#include "ospray/OSPEnums.h"
#include "render/Renderer.ih"
#include "render/ScreenSample.ih"
#include "render/util.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

/* Return the position of the screen sample within its tile
 */
inline vec2i SparseFB_getTilePixelPos(SparseFB *uniform fb,
    const ScreenSample &screenSample,
    const uniform uint32 tileIdx)
{
  uniform vec2i tilePos = fb->tiles[tileIdx].region.lower;
  const vec2i tilePixelPos =
      make_vec2i(screenSample.sampleID.x, screenSample.sampleID.y) - tilePos;
  return tilePixelPos;
}

// Accumulates the screenSample color and alpha into the framebuffer's
// accumulation buffer.
// I.e., the color to be written to the final framebuffer for display for
// the current frame
//
// tileIdx is the index of the tile that this task/sample belongs to, as
// computed by SparseFB_getTileIndexForTask. sampleTilePos is the position of
// the sample within this tile, via SparseFB_getTilePixelPos
inline void SparseFB_accumulateTileSample(SparseFB *uniform fb,
    const ScreenSample &screenSample,
    const uniform uint32 tileIdx,
    const vec2i &sampleTilePos,
    uniform RenderTaskDesc &taskDesc)
{
  // The location of this pixel's data within the tiled accumulation and
  // variance buffers
  const uint32 tilePixelIdx = sampleTilePos.x + TILE_SIZE * sampleTilePos.y;

  const vec4f sampleValue = make_vec4f(screenSample.rgb, screenSample.alpha);
  uniform Tile *uniform tile = fb->tiles + tileIdx;

  const uniform int accumID = fb->accumulate ? fb->super.frameID : 0;
  const uniform float accScale = rcpf(accumID + 1);

  // Accumulate color
  vec4f color = make_vec4f(tile->r[tilePixelIdx],
      tile->g[tilePixelIdx],
      tile->b[tilePixelIdx],
      tile->a[tilePixelIdx]);
  FRAMEBUFFER_ACCUMULATE_VALUE(color, sampleValue, accScale);
  tile->r[tilePixelIdx] = color.x;
  tile->g[tilePixelIdx] = color.y;
  tile->b[tilePixelIdx] = color.z;
  tile->a[tilePixelIdx] = color.w;

  // Accumulate depth
  float depth = screenSample.z;
  if (accumID > 0)
    depth = min(tile->z[tilePixelIdx], screenSample.z);
  tile->z[tilePixelIdx] = depth;

  // Accumulate normal
  vec3f normal = make_vec3f(
      tile->nx[tilePixelIdx], tile->ny[tilePixelIdx], tile->nz[tilePixelIdx]);
  FRAMEBUFFER_ACCUMULATE_VALUE(normal, screenSample.normal, accScale);
  tile->nx[tilePixelIdx] = normal.x;
  tile->ny[tilePixelIdx] = normal.y;
  tile->nz[tilePixelIdx] = normal.z;

  // Accumulate albedo
  vec3f albedo = make_vec3f(
      tile->ar[tilePixelIdx], tile->ag[tilePixelIdx], tile->ab[tilePixelIdx]);
  FRAMEBUFFER_ACCUMULATE_VALUE(albedo, screenSample.albedo, accScale);
  tile->ar[tilePixelIdx] = albedo.x;
  tile->ag[tilePixelIdx] = albedo.y;
  tile->ab[tilePixelIdx] = albedo.z;

  tile->pid[tilePixelIdx] = screenSample.primID;
  tile->gid[tilePixelIdx] = screenSample.geomID;
  tile->iid[tilePixelIdx] = screenSample.instID;

  uniform vec4f *uniform variance = fb->varianceBuffer;
  if (fb->super.accumulateVariance) {
    const uniform float accVarScale = rcpf(accumID / 2 + 1);
    const uint32 varianceAccumIndex = tileIdx * TILE_SIZE * TILE_SIZE
        + sampleTilePos.x + TILE_SIZE * sampleTilePos.y;
    FRAMEBUFFER_ACCUMULATE_VALUE(
        variance[varianceAccumIndex], sampleValue, accVarScale);

    // Calculate error
    float err = 0.f;
    int cnt = 0;
    const float den2 = reduce_add(make_vec3f(color))
        + (1.f - color.w); // invert alpha (bright alpha is more important)
    if (den2 > 0.f) {
      const vec4f diff = absf(color - variance[varianceAccumIndex]);
      err = reduce_add(diff) * rsqrtf(den2);
      cnt = 1;
    }
    // We max over the task because we may have more pixels in the task than
    // the SIMD width
    taskDesc.error += reduce_add(err);
    taskDesc.count += reduce_add(cnt);
  }
}

SYCL_EXTERNAL void SparseFB_accumulateSample(FrameBuffer *uniform _fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &taskDesc)
{
  SparseFB *uniform fb = (SparseFB * uniform) _fb;

  // tileIdx -> index in the SparseFB's list of tiles
  const uniform uint32 tileIdx =
      SparseFB_getTileIndexForTask(fb, taskDesc.taskID);
  const vec2i sampleTilePos =
      SparseFB_getTilePixelPos(fb, screenSample, tileIdx);

  SparseFB_accumulateTileSample(
      fb, screenSample, tileIdx, sampleTilePos, taskDesc);

#ifndef OSPRAY_TARGET_SYCL
  FrameBuffer_updateProgress(&fb->super, popcnt(lanemask()));
#endif
}

SYCL_EXTERNAL uniform RenderTaskDesc SparseFB_getRenderTaskDesc(
    FrameBuffer *uniform _fb, const uniform uint32 taskID)
{
  SparseFB *uniform fb = (SparseFB * uniform) _fb;
  // Map to the tile and then the set of pixels within that tile.
  uniform RenderTaskDesc desc;
  desc.taskID = taskID;
  desc.error = 0.f;
  desc.count = 0;

  const uniform uint32 tileIdx = SparseFB_getTileIndexForTask(fb, taskID);

  // Find where this task is supposed to render within this tile
  const uniform vec2i tileDims = make_vec2i(TILE_SIZE);
  const uniform vec2i tasksPerTile = tileDims / fb->super.renderTaskSize;
  const uniform uint32 taskTileID = taskID % (tasksPerTile.x * tasksPerTile.y);

  uniform vec2i taskStart =
      make_vec2i(taskTileID % tasksPerTile.x, taskTileID / tasksPerTile.x);
  desc.region.lower = taskStart * fb->super.renderTaskSize;
  desc.region.upper = desc.region.lower + fb->super.renderTaskSize;

  // Offset the task's region by the tile location
  const uniform vec2i tilePos = fb->tiles[tileIdx].region.lower;
  desc.region.lower = desc.region.lower + tilePos;
  desc.region.upper = min(desc.region.upper + tilePos, fb->super.size);

  // TODO: In the end, shouldn't generate these out of bounds tasks for the
  // padded framebuffer region? For now just mark them as having no error
  // In the past the tiled load balancer would also generate these though, with
  // how we padded out to the tile dims
  if (isEmpty(desc.region) && fb->taskRegionError) {
    fb->taskRegionError[taskID] = 0.f;
  }

  return desc;
}

SYCL_EXTERNAL void SparseFB_completeTask(
    FrameBuffer *uniform _fb, const uniform RenderTaskDesc &taskDesc)
{
  // Write error for the task
  SparseFB *uniform fb = (SparseFB * uniform) _fb;
  if (fb->super.accumulateVariance) {
    uniform float err = taskDesc.error;
    if (taskDesc.count)
      err *= rcp((uniform float)taskDesc.count); // avg

    // scale to be backwards compatible with the old default tile size of 64x64
    fb->taskRegionError[taskDesc.taskID] = err * 64.f;
  }
}

#ifndef OSPRAY_TARGET_SYCL
export void *uniform SparseFrameBuffer_accumulateSample_addr()
{
  return (void *uniform)SparseFB_accumulateSample;
}

export void *uniform SparseFrameBuffer_getRenderTaskDesc_addr()
{
  return (void *uniform)SparseFB_getRenderTaskDesc;
}

export void *uniform SparseFrameBuffer_completeTask_addr()
{
  return (void *uniform)SparseFB_completeTask;
}
#endif
OSPRAY_END_ISPC_NAMESPACE
