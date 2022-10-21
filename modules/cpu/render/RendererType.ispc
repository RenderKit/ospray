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

#ifndef OSPRAY_TARGET_SYCL
void Renderer_dispatch_renderSample(
    Renderer *uniform self,
    FrameBuffer *uniform fb,
    World *uniform model,
    void *uniform perFrameData,
    varying ScreenSample &sample)
{
  switch (self->type) {
  case RENDERER_TYPE_DEBUG:
    DebugRenderer_renderSample(self, fb, model, perFrameData, sample);
    break;
  case RENDERER_TYPE_AO:
    AORenderer_renderSample(self, fb, model, perFrameData, sample);
    break;
  case RENDERER_TYPE_SCIVIS:
    SciVis_renderSample(self, fb, model, perFrameData, sample);
    break;
  default:
    self->renderSample(self, fb, model, perFrameData, sample);
    break;
  }
}
#endif

OSPRAY_END_ISPC_NAMESPACE

