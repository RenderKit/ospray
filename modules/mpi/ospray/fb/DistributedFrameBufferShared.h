// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/FrameBufferShared.h"
#include "fb/TileShared.h"

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

#ifdef OSPRAY_TARGET_SYCL

float DFB_accumulateTile(const VaryingTile *uniform tile,
    VaryingTile *uniform final,
    VaryingTile *uniform accum,
    VaryingTile *uniform variance,
    uniform bool hasAccumBuffer,
    uniform bool hasVarianceBuffer);

void DFB_accumulateTileSimple(const VaryingTile *uniform tile,
    VaryingTile *uniform accum,
    VaryingTile *uniform variance);

void DFB_accumulateAuxTile(const VaryingTile *uniform tile,
    void *uniform _final,
    VaryingTile *uniform accum);

float DFB_computeErrorForTile(const uniform vec2i &size,
    const VaryingTile *uniform accum,
    const VaryingTile *uniform variance,
    const uniform float accumID);

void DFB_runPixelOpsForTile(void *_self, void *_tile);

void DFB_writeTile_RGBA8(const VaryingTile *tile, void *_color);
void DFB_writeTile_SRGBA(const VaryingTile *tile, void *_color);
void DFB_writeTile_RGBA32F(const VaryingTile *tile, void *_color);

void DFB_sortAndBlendFragments(
    VaryingTile *uniform *uniform tileArray, uniform int32 numTiles);
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
