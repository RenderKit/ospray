// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "OSPConfig.h"

#ifdef __cplusplus
namespace ispc {
#endif

#ifdef __cplusplus
static_assert(TILE_SIZE > 0 && (TILE_SIZE & (TILE_SIZE - 1)) == 0,
    "OSPRay config error: TILE_SIZE must be a positive power of two.");
#endif

// Extra defines to hide some differences between ISPC/C++ to unify the
// declaration
// TODO: remove all uniform/varying modifiers from Tile and VaryingTile
#ifdef __cplusplus
#ifndef OSPRAY_TARGET_SYCL
#define uniform
#endif
#else
#define OSPRAY_SDK_INTERFACE
#endif

#ifndef OSPRAY_SDK_INTERFACE
#define OSPRAY_SDK_INTERFACE
#endif

//! a tile of pixels used by any tile-based renderer
/*! pixels in the tile are in a row-major TILE_SIZE x TILE_SIZE
    pattern. the 'region' specifies which part of the screen this
    tile belongs to: tile.lower is the lower-left coordinate of this
    tile (and a multiple of TILE_SIZE); the 'upper' value may be
    smaller than the upper-right edge of the "full" tile.

    note that a tile contains "all" of the values a renderer might
    want to use. not all renderers nor all frame buffers will use
    all those values; it's up to the renderer and frame buffer to
    agree on which fields will be set. Similarly, the frame buffer
    may actually use uchars, but the tile will always store
    floats. */
struct OSPRAY_SDK_INTERFACE Tile
{
  uniform range2i region;
  uniform vec2i fbSize;
  uniform vec2f rcp_fbSize;
  uniform int32 generation;
  uniform int32 children;
  uniform int32 sortOrder;
  uniform int32 accumID;
  uniform float pad[4]; // padding to match the varying tile layout
  uniform float r[TILE_SIZE * TILE_SIZE]; // red
  uniform float g[TILE_SIZE * TILE_SIZE]; // green
  uniform float b[TILE_SIZE * TILE_SIZE]; // blue
  uniform float a[TILE_SIZE * TILE_SIZE]; // alpha
  uniform float z[TILE_SIZE * TILE_SIZE]; // depth
  uniform float nx[TILE_SIZE * TILE_SIZE]; // normal x
  uniform float ny[TILE_SIZE * TILE_SIZE]; // normal y
  uniform float nz[TILE_SIZE * TILE_SIZE]; // normal z
  uniform float ar[TILE_SIZE * TILE_SIZE]; // albedo red
  uniform float ag[TILE_SIZE * TILE_SIZE]; // albedo green
  uniform float ab[TILE_SIZE * TILE_SIZE]; // albedo blue
  uniform uint32 pid[TILE_SIZE * TILE_SIZE]; // primID
  uniform uint32 gid[TILE_SIZE * TILE_SIZE]; // objID
  uniform uint32 iid[TILE_SIZE * TILE_SIZE]; // instanceID

#ifdef __cplusplus
  Tile()
      : region(empty),
        fbSize(0),
        rcp_fbSize(0.f),
        generation(0),
        children(0),
        sortOrder(0),
        accumID(0)
  {}

  Tile(const vec2i &tile, const vec2i &fbsize, const int32 accumId)
      : fbSize(fbsize), accumID(accumId)
  {
    rcp_fbSize.x = rcp(float(fbSize.x));
    rcp_fbSize.y = rcp(float(fbSize.y));
    region.lower = tile * TILE_SIZE;
    region.upper = min(region.lower + TILE_SIZE, fbsize);
  }
#endif
};

#if !defined(__cplusplus) || defined(OSPRAY_TARGET_SYCL)
struct VaryingTile
{
  uniform range2i region; // 4 ints
  uniform vec2i fbSize; // 2 ints
  uniform vec2f rcp_fbSize; // 2 floats
  uniform int32 generation;
  uniform int32 children;
  uniform int32 sortOrder;
  uniform int32 accumID;
  uniform float pad[4]; // explicit padding to match on SSE, this padding is
                        // implicitly added on AVX and AVX512 to align the
                        // vectors though. We need it here to match on SSE
  varying float r[TILE_SIZE * TILE_SIZE / programCount];
  varying float g[TILE_SIZE * TILE_SIZE / programCount];
  varying float b[TILE_SIZE * TILE_SIZE / programCount];
  varying float a[TILE_SIZE * TILE_SIZE / programCount];
  varying float z[TILE_SIZE * TILE_SIZE / programCount];
  varying float nx[TILE_SIZE * TILE_SIZE / programCount];
  varying float ny[TILE_SIZE * TILE_SIZE / programCount];
  varying float nz[TILE_SIZE * TILE_SIZE / programCount];
  varying float ar[TILE_SIZE * TILE_SIZE / programCount];
  varying float ag[TILE_SIZE * TILE_SIZE / programCount];
  varying float ab[TILE_SIZE * TILE_SIZE / programCount];
  varying uint32 pid[TILE_SIZE * TILE_SIZE / programCount]; // primID
  varying uint32 gid[TILE_SIZE * TILE_SIZE / programCount]; // objID
  varying uint32 iid[TILE_SIZE * TILE_SIZE / programCount]; // instanceID
};
#endif

#ifdef __cplusplus
#ifndef OSPRAY_TARGET_SYCL
#undef uniform
#endif
#else
#undef OSPRAY_SDK_INTERFACE
#endif

#ifdef __cplusplus
}
namespace ospray {
using Tile = ispc::Tile;
}
#endif
