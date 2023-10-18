// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FrameBufferShared.h"
#include "TileShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// a SparseFrameBuffer stores a subset of the full framebuffer as Tiles
struct SparseFB
{
  // superclass that we inherit from
  FrameBuffer super;
  // Number of tasks being used to render the image in x & y
  vec2i numRenderTasks;
  // The total number of tiles that the framebuffer is divided into
  vec2i totalTiles;
  uint32 numTiles;
  // Image data for the tiles in this SparseFrameBuffer
  Tile *tiles;
  // holds accumID per task region, for adaptive accumulation
  int32 *taskAccumID;
  // holds the accumulated color for each tile, tile's rgba stores the final
  // color
  vec4f *accumulationBuffer;
  // accumulates every other sample, for variance estimation / stopping
  vec4f *varianceBuffer;
  // holds error per task region, for adaptive accumulation
  float *taskRegionError;

#ifdef __cplusplus
  SparseFB()
      : numRenderTasks(0),
        totalTiles(0),
        numTiles(0),
        tiles(nullptr),
        taskAccumID(nullptr),
        accumulationBuffer(nullptr),
        varianceBuffer(nullptr),
        taskRegionError(nullptr)
  {
    super.type = FRAMEBUFFER_TYPE_SPARSE;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
