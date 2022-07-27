// Copyright 2009 Intel Corporation
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
struct SparseFB;

typedef task void (*DR_ComputeRegionVisibility)(
    DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    uint8 *uniform regionVisible,
    void *uniform perFrameData,
    const int *uniform taskIDs);

typedef void (*DR_RenderRegionSampleFct)(DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    const vec2f &regionInterval,
    void *uniform perFrameData,
    varying ScreenSample &sample);

typedef task void (*DR_RenderRegionTileFct)(DistributedRenderer *uniform self,
    SparseFB *uniform fb,
    Camera *uniform camera,
    DistributedWorld *uniform world,
    const box3f *uniform region,
    void *uniform perFrameData,
    const int *uniform taskIDs);
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
