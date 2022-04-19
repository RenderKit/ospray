// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameBufferShared.h"
#include "fb/FrameBufferType.ih"
#include "fb/RenderTaskDesc.ih"
#include "render/ScreenSample.ih"

#include "fb/LocalFB.ih"
#include "fb/SparseFB.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline uniform RenderTaskDesc
FrameBuffer_dispatch_getRenderTaskDesc(
    FrameBuffer *uniform fb, const uniform uint32 taskID)
{
  switch (fb->type) {
  case FRAMEBUFFER_TYPE_LOCAL:
    return LocalFB_getRenderTaskDesc(fb, taskID);
  case FRAMEBUFFER_TYPE_SPARSE:
    return SparseFB_getRenderTaskDesc(fb, taskID);
  default:
#ifndef OSPRAY_TARGET_DPCPP
    return fb->getRenderTaskDesc(fb, taskID);
#else
    break;
#endif
  }
  // TODO: Should be an error here
  uniform RenderTaskDesc rt;
  rt.region.lower = make_vec2i(0, 0);
  rt.region.upper = rt.region.lower;
  return rt;
}

SYCL_EXTERNAL __noinline void FrameBuffer_dispatch_accumulateSample(
    FrameBuffer *uniform fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &taskDesc)
{
  switch (fb->type) {
  case FRAMEBUFFER_TYPE_LOCAL:
    LocalFB_accumulateSample(fb, screenSample, taskDesc);
    break;
  case FRAMEBUFFER_TYPE_SPARSE:
    SparseFB_accumulateSample(fb, screenSample, taskDesc);
    break;
  default:
#ifndef OSPRAY_TARGET_DPCPP
    fb->accumulateSample(fb, screenSample, taskDesc);
#endif
    break;
  }
}

SYCL_EXTERNAL __noinline void FrameBuffer_dispatch_completeTask(
    FrameBuffer *uniform fb, const uniform RenderTaskDesc &taskDesc)
{
  switch (fb->type) {
  case FRAMEBUFFER_TYPE_LOCAL:
    LocalFB_completeTask(fb, taskDesc);
    break;
  case FRAMEBUFFER_TYPE_SPARSE:
    SparseFB_completeTask(fb, taskDesc);
    break;
  default:
#ifndef OSPRAY_TARGET_DPCPP
    fb->completeTask(fb, taskDesc);
#endif
    break;
  }
}

OSPRAY_END_ISPC_NAMESPACE

