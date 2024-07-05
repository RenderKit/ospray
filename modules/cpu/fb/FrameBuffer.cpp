// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"
#include "FrameOp.h"
#include "OSPConfig.h"
#include "fb/FrameBufferView.h"
#ifndef OSPRAY_TARGET_SYCL
#include "ISPCDevice_ispc.h"
#endif

namespace {
// Internal utilities for thread local progress tracking
thread_local int threadLastFrameID = -1;
thread_local uint32_t threadNumPixelsRendered = 0;

extern "C" uint32_t *getThreadNumPixelsRendered()
{
  return &threadNumPixelsRendered;
}

extern "C" int *getThreadLastFrameID()
{
  return &threadLastFrameID;
}

}; // namespace

namespace ospray {

FrameBuffer::FrameBuffer(api::ISPCDevice &device,
    const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels,
    const FeatureFlagsOther ffo)
    : AddStructShared(device.getDRTDevice(), device),
      size(_size),
      colorBufferFormat(_colorBufferFormat),
      hasColorBuffer(channels & OSP_FB_COLOR),
      hasDepthBuffer(channels & OSP_FB_DEPTH),
      hasVarianceBuffer((channels & OSP_FB_COLOR)
          && (channels & OSP_FB_VARIANCE) && (channels & OSP_FB_ACCUM)),
      hasNormalBuffer(channels & OSP_FB_NORMAL),
      hasAlbedoBuffer(channels & OSP_FB_ALBEDO),
      hasPrimitiveIDBuffer(channels & OSP_FB_ID_PRIMITIVE),
      hasObjectIDBuffer(channels & OSP_FB_ID_OBJECT),
      hasInstanceIDBuffer(channels & OSP_FB_ID_INSTANCE),
      doAccum(channels & OSP_FB_ACCUM),
      featureFlags(ffo)
{
  managedObjectType = OSP_FRAMEBUFFER;
  if (_size.x <= 0 || _size.y <= 0) {
    throw std::runtime_error(
        "framebuffer has invalid size. Dimensions must be greater than 0");
  }
  getSh()->size = _size;
  getSh()->rcpSize = vec2f(1.f) / vec2f(_size);
  getSh()->channels = channels;
  getSh()->accumulateVariance = false;
  getSh()->targetFrames = !doAccum;

#if OSPRAY_RENDER_TASK_SIZE == -1
#ifdef OSPRAY_TARGET_SYCL
  vec2i renderTaskSize(8);
#else
  // Compute render task size based on the simd width to get as "square" as
  // possible a task size that has simdWidth pixels
  const int simdWidth = ispc::ISPCDevice_programCount();
  vec2i renderTaskSize(simdWidth, 1);
  while (renderTaskSize.x / 2 > renderTaskSize.y) {
    renderTaskSize.y *= 2;
    renderTaskSize.x /= 2;
  }
#endif
#else
  // Note: we could also allow changing this at runtime if we want to add this
  // to the API
  vec2i renderTaskSize(OSPRAY_RENDER_TASK_SIZE);
#endif
  getSh()->renderTaskSize = renderTaskSize;
}

void FrameBuffer::commit()
{
  imageOpData = getParamDataT<ImageOp *>("imageOperation");
  // always query `targetFrames` to clear query status
  getSh()->targetFrames = max(0, getParam<int>("targetFrames", 0));
  if (!doAccum)
    getSh()->targetFrames = 1;
}

void FrameBuffer::clear()
{
  getSh()->frameID = -1; // we increment at the start of the frame

  if (hasVarianceBuffer) {
    mtGen.seed(mtSeed); // reset the RNG with same seed
    frameVariance = inf;
  }
}

vec2i FrameBuffer::getRenderTaskSize() const
{
  return getSh()->renderTaskSize;
}

float FrameBuffer::getVariance() const
{
  return frameVariance;
}

void FrameBuffer::beginFrame()
{
  cancelRender = false;
  getSh()->frameID = doAccum ? getSh()->frameID + 1 : 0;
#ifndef OSPRAY_TARGET_SYCL
  // TODO: Cancellation isn't supported on the GPU
  getSh()->cancelRender = 0;
  // TODO: Maybe better as a kernel to avoid USM thrash to host
  getSh()->numPixelsRendered = 0;
#endif

  if (hasVarianceBuffer) {
    // collect half of the samples
    // To not run into correlation issues with low discrepancy sequences used
    // in renders, we randomly pick whether the even or the odd frame is
    // accumulated into the variance buffer.
    const bool evenFrame = (getSh()->frameID & 1) == 0;
    if (evenFrame)
      pickOdd = mtGen() & 1; // coin flip every other frame

    getSh()->accumulateVariance = evenFrame != pickOdd;
  }
}

std::string FrameBuffer::toString() const
{
  return "ospray::FrameBuffer";
}

void FrameBuffer::setCompletedEvent(OSPSyncEvent event)
{
#ifndef OSPRAY_TARGET_SYCL
  // We won't be running ISPC-side rendering tasks when updating the
  // progress values here in C++
  if (event == OSP_NONE_FINISHED)
    getSh()->numPixelsRendered = 0;
  if (event == OSP_FRAME_FINISHED)
    getSh()->numPixelsRendered = getNumPixels().long_product();
#endif
  stagesCompleted = event;
}

OSPSyncEvent FrameBuffer::getLatestCompleteEvent() const
{
  return stagesCompleted;
}

void FrameBuffer::waitForEvent(OSPSyncEvent event) const
{
  // TODO: condition variable to sleep calling thread instead of spinning?
  while (stagesCompleted < event)
    ;
}

float FrameBuffer::getCurrentProgress() const
{
#ifdef OSPRAY_TARGET_SYCL
  // TODO: Continually polling this will cause a lot of USM thrashing
  return 0.f;
#else
  return static_cast<float>(getSh()->numPixelsRendered)
      / getNumPixels().long_product();
#endif
}

void FrameBuffer::cancelFrame()
{
  cancelRender = true;
  // TODO: Cancellation isn't supported on the GPU
#ifndef OSPRAY_TARGET_SYCL
  getSh()->cancelRender = 1;
#endif
}

bool FrameBuffer::frameCancelled() const
{
  return cancelRender;
}

uint32 FrameBuffer::getChannelFlags() const
{
  return getSh()->channels;
}

OSPTYPEFOR_DEFINITION(FrameBuffer *);

} // namespace ospray
