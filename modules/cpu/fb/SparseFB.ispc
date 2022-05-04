// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SparseFB.ih"
#include "fb/FrameBuffer.ih"
#include "fb/RenderTaskDesc.ih"
#include "fb/Tile.ih"
#include "ospray/OSPEnums.h"
#include "render/Renderer.ih"
#include "render/util.ih"

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

/* Accumulates the screenSample color and alpha into the framebuffer's
   accumulation buffer. The accumulation buffer will store

       prevAccum + sampleValue

   while the returned current accumulated color will be

      (prevAccum + sampleValue) / numAccumulated

   I.e., the color to be written to the final framebuffer for display for
   the current frame

   tileIdx is the index of the tile that this task/sample belongs to, as
   computed by SparseFB_getTileIndexForTask. sampleTilePos is the position of
   the sample within this tile, via SparseFB_getTilePixelPos
 */
inline void SparseFB_accumulateTileSample(SparseFB *uniform fb,
    const ScreenSample &screenSample,
    const uniform uint32 tileIdx,
    const vec2i &sampleTilePos,
    uniform RenderTaskDesc &taskDesc)
{
  uniform vec4f *uniform accumulation = fb->accumulationBuffer;
  uniform vec4f *uniform variance = fb->varianceBuffer;
  // The location of this pixel's data within the tiled accumulation and
  // variance buffers
  const uint32 varianceAccumIndex = tileIdx * TILE_SIZE * TILE_SIZE
      + sampleTilePos.x + TILE_SIZE * sampleTilePos.y;
  const uint32 tilePixelIdx = sampleTilePos.x + TILE_SIZE * sampleTilePos.y;

  const uniform float accScale = rcpf(taskDesc.accumID + 1);
  const uniform float accHalfScale = rcpf(taskDesc.accumID / 2 + 1);

  const vec4f sampleValue = make_vec4f(screenSample.rgb, screenSample.alpha);
  uniform Tile *uniform tile = fb->tiles + tileIdx;

  vec4f acc = sampleValue;
  float depth = screenSample.z;
  vec3f normal = screenSample.normal;
  vec3f albedo = screenSample.albedo;

  if (accumulation) {
    if (taskDesc.accumID > 0) {
      acc = accumulation[varianceAccumIndex] + sampleValue;

      depth = min(tile->z[tilePixelIdx], screenSample.z);

      normal = make_vec3f(tile->nx[tilePixelIdx],
          tile->ny[tilePixelIdx],
          tile->nz[tilePixelIdx]);
      normal = (normal * taskDesc.accumID + screenSample.normal) * accScale;

      albedo = make_vec3f(tile->ar[tilePixelIdx],
          tile->ag[tilePixelIdx],
          tile->ab[tilePixelIdx]);
      albedo = (albedo * taskDesc.accumID + screenSample.albedo) * accScale;
    }
    accumulation[varianceAccumIndex] = acc;
    acc = acc * accScale;
  }

  const vec4f finalColor =
      FrameBuffer_runPixelOps(&fb->super, acc, depth, normal, albedo);

  tile->r[tilePixelIdx] = finalColor.x;
  tile->g[tilePixelIdx] = finalColor.y;
  tile->b[tilePixelIdx] = finalColor.z;
  tile->a[tilePixelIdx] = finalColor.w;

  tile->z[tilePixelIdx] = depth;

  tile->nx[tilePixelIdx] = normal.x;
  tile->ny[tilePixelIdx] = normal.y;
  tile->nz[tilePixelIdx] = normal.z;

  tile->ar[tilePixelIdx] = albedo.x;
  tile->ag[tilePixelIdx] = albedo.y;
  tile->ab[tilePixelIdx] = albedo.z;

  tile->pid[tilePixelIdx] = screenSample.primID;
  tile->gid[tilePixelIdx] = screenSample.geomID;
  tile->iid[tilePixelIdx] = screenSample.instID;

  // Variance and error accumulates every other frame  to ensure we get a
  // monotonically decreasing sequence of error values
  if (variance && (taskDesc.accumID & 1)) {
    vec4f vari = make_vec4f(0.f);
    if (taskDesc.accumID > 1) {
      vari = variance[varianceAccumIndex];
    }
    vari = vari + sampleValue;
    variance[varianceAccumIndex] = vari;

    float err = 0.f;
    const float den2 = reduce_add(make_vec3f(acc)) + (1.f - acc.w);
    if (den2 > 0.f) {
      const vec4f diff = absf(acc - accHalfScale * vari);
      err += reduce_add(diff) * rsqrt(den2);
    }
    // We sum over the task because we may have more pixels in the task than
    // SIMD width
    taskDesc.error += reduce_add(err);
  }
}

void SparseFB_accumulateSample(FrameBuffer *uniform _fb,
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

  FrameBuffer_updateProgress(&fb->super, popcnt(lanemask()));
}

uniform RenderTaskDesc SparseFB_getRenderTaskDesc(
    FrameBuffer *uniform _fb, const uniform uint32 taskID)
{
  SparseFB *uniform fb = (SparseFB * uniform) _fb;
  // Map to the tile and then the set of pixels within that tile.
  uniform RenderTaskDesc desc;
  desc.taskID = taskID;
  desc.accumID = fb->taskAccumID ? fb->taskAccumID[taskID] : 0;
  desc.error = 0.f;

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

void SparseFB_completeTask(
    FrameBuffer *uniform _fb, const uniform RenderTaskDesc &taskDesc)
{
  SparseFB *uniform fb = (SparseFB * uniform) _fb;

  // Write error for the task every other frame to get a monotonically
  // decreasing sequence of error values
  if (fb->varianceBuffer && (taskDesc.accumID & 1)) {
    // Scale to be backwards compatible with the old default tile/task
    // size of 64x64
    const uniform vec2i dia = taskDesc.region.upper - taskDesc.region.lower;
    const uniform float taskSize = dia.x * dia.y;

    fb->taskRegionError[taskDesc.taskID] =
        taskDesc.error * rcp(taskSize) * 64.f;
  }

  // Increment accumID for this task
  if (fb->taskAccumID) {
    fb->taskAccumID[taskDesc.taskID] = taskDesc.accumID + 1;
  }
}

export void *uniform SparseFrameBuffer_accumulateSample_addr()
{
  return SparseFB_accumulateSample;
}

export void *uniform SparseFrameBuffer_getRenderTaskDesc_addr()
{
  return SparseFB_getRenderTaskDesc;
}

export void *uniform SparseFrameBuffer_completeTask_addr()
{
  return SparseFB_completeTask;
}
