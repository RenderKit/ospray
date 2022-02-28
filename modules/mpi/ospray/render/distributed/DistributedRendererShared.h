// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
typedef void *DR_ComputeRegionVisibility;
typedef void *DR_RenderRegionSampleFct;
typedef void *DR_RenderRegionTileFct;
#else
struct DistributedWorld;
struct DistributedRenderer;

typedef unmasked void (*DR_ComputeRegionVisibility)(
    DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    bool *uniform regionVisible,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex);

typedef void (*DR_RenderRegionSampleFct)(DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    const vec2f &regionInterval,
    void *uniform perFrameData,
    varying ScreenSample &sample);

typedef unmasked void (*DR_RenderRegionTileFct)(
    DistributedRenderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    void *uniform perFrameData,
    uniform Tile &tile,
    uniform int taskIndex);
#endif // __cplusplus

struct DistributedRenderer
{
  Renderer super;

  DR_ComputeRegionVisibility computeRegionVisibility;
  DR_RenderRegionSampleFct renderRegionSample;
  DR_RenderRegionTileFct renderRegionToTile;

#ifdef __cplusplus
  DistributedRenderer()
      : computeRegionVisibility(nullptr),
        renderRegionSample(nullptr),
        renderRegionToTile(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
