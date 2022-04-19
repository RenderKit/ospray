// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RendererType.ih"
#include "common/WorldShared.h"
#include "fb/FrameBufferShared.h"
#include "render/RendererShared.h"

#include "ao/AORenderer.ih"
#include "debug/DebugRenderer.ih"
#include "scivis/SciVis.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

RTC_SYCL_INDIRECTLY_CALLABLE void Renderer_dispatch_renderSample(
    Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform model,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  switch (self->type) {
  case RENDERER_TYPE_DEBUG:
    // TODO: Cannot have call to both debug renderer and AO renderer here
    // because it leads to a hang
    DebugRenderer_renderSample(self, fb, model, perFrameData, sample);
    break;
  case RENDERER_TYPE_AO:
    AORenderer_renderSample(self, fb, model, perFrameData, sample);
    break;
  case RENDERER_TYPE_SCIVIS:
    SciVis_renderSample(self, fb, model, perFrameData, sample);
    break;
  default:
    // TODO: No function pointer for DPCPP version because it'll hang.
    // Should we have an error here or still allow it to be called?
#ifndef OSPRAY_TARGET_DPCPP
    self->renderSample(self, fb, model, perFrameData, sample);
#endif
    break;
  }
}

OSPRAY_END_ISPC_NAMESPACE

