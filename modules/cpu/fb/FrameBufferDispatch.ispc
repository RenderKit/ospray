// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/FeatureFlagsEnum.h"
#include "fb/FrameBufferDispatch.ih"
#include "fb/LocalFB.ih"
#include "fb/RenderTaskDesc.ih"
#include "fb/SparseFB.ih"
#include "render/ScreenSample.ih"
// c++ shared
#include "fb/FrameBufferShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL uniform RenderTaskDesc FrameBuffer_dispatch_getRenderTaskDesc(
    FrameBuffer *uniform fb,
    const uniform uint32 taskID,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((fb->type == FRAMEBUFFER_TYPE_LOCAL) && (ffo & FFO_FB_LOCAL)) {
    return LocalFB_getRenderTaskDesc(fb, taskID);
  } else if ((fb->type == FRAMEBUFFER_TYPE_SPARSE) && (ffo & FFO_FB_SPARSE)) {
    return SparseFB_getRenderTaskDesc(fb, taskID);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return fb->getRenderTaskDesc(fb, taskID);
#endif
  }
  // TODO: Should be an error here
  uniform RenderTaskDesc rt;
  rt.region.lower = make_vec2i(0, 0);
  rt.region.upper = rt.region.lower;
  return rt;
}

SYCL_EXTERNAL void FrameBuffer_dispatch_accumulateSample(
    FrameBuffer *uniform fb,
    const varying ScreenSample &screenSample,
    uniform RenderTaskDesc &taskDesc,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((fb->type == FRAMEBUFFER_TYPE_LOCAL) && (ffo & FFO_FB_LOCAL)) {
    LocalFB_accumulateSample(fb, screenSample, taskDesc);
  } else if ((fb->type == FRAMEBUFFER_TYPE_SPARSE) && (ffo & FFO_FB_SPARSE)) {
    SparseFB_accumulateSample(fb, screenSample, taskDesc);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    fb->accumulateSample(fb, screenSample, taskDesc);
#endif
  }
}

SYCL_EXTERNAL void FrameBuffer_dispatch_completeTask(FrameBuffer *uniform fb,
    const uniform RenderTaskDesc &taskDesc,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((fb->type == FRAMEBUFFER_TYPE_LOCAL) && (ffo & FFO_FB_LOCAL)) {
    LocalFB_completeTask(fb, taskDesc);
  } else if ((fb->type == FRAMEBUFFER_TYPE_SPARSE) && (ffo & FFO_FB_SPARSE)) {
    SparseFB_completeTask(fb, taskDesc);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    fb->completeTask(fb, taskDesc);
#endif
  }
}

OSPRAY_END_ISPC_NAMESPACE
