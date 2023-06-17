// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"
#include "FrameOp.h"
#include "OSPConfig.h"
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
    : AddStructShared(device.getIspcrtContext(), device),
      size(_size),
      hasDepthBuffer(channels & OSP_FB_DEPTH),
      hasAccumBuffer(channels & OSP_FB_ACCUM),
      hasVarianceBuffer(
          (channels & OSP_FB_VARIANCE) && (channels & OSP_FB_ACCUM)),
      hasNormalBuffer(channels & OSP_FB_NORMAL),
      hasAlbedoBuffer(channels & OSP_FB_ALBEDO),
      hasPrimitiveIDBuffer(channels & OSP_FB_ID_PRIMITIVE),
      hasObjectIDBuffer(channels & OSP_FB_ID_OBJECT),
      hasInstanceIDBuffer(channels & OSP_FB_ID_INSTANCE),
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
  getSh()->colorBufferFormat = _colorBufferFormat;

#ifdef OSPRAY_TARGET_SYCL
  // Note: using 2x2, 4x4, etc doesn't change perf much
  vec2i renderTaskSize(1);
#else
#if OSPRAY_RENDER_TASK_SIZE == -1
  // Compute render task size based on the simd width to get as "square" as
  // possible a task size that has simdWidth pixels
  const int simdWidth = ispc::ISPCDevice_programCount();
  vec2i renderTaskSize(simdWidth, 1);
  while (renderTaskSize.x / 2 > renderTaskSize.y) {
    renderTaskSize.y *= 2;
    renderTaskSize.x /= 2;
  }
#else
  // Note: we could also allow changing this at runtime if we want to add this
  // to the API
  vec2i renderTaskSize(OSPRAY_RENDER_TASK_SIZE);
#endif
#endif
  getSh()->renderTaskSize = renderTaskSize;
}

void FrameBuffer::commit()
{
  // Erase all image operations arrays
  frameOps.clear();
  pixelOps.clear();
  pixelOpShs.clear();
  getSh()->pixelOps = nullptr;
  getSh()->numPixelOps = 0;

  // Read image operations array set by user
  imageOpData = getParamDataT<ImageOp *>("imageOperation");
}

void FrameBuffer::clear()
{
  frameID = -1; // we increment at the start of the frame
}

vec2i FrameBuffer::getRenderTaskSize() const
{
  return getSh()->renderTaskSize;
}

vec2i FrameBuffer::getNumPixels() const
{
  return size;
}

OSPFrameBufferFormat FrameBuffer::getColorBufferFormat() const
{
  return getSh()->colorBufferFormat;
}

float FrameBuffer::getVariance() const
{
  return frameVariance;
}

void FrameBuffer::beginFrame()
{
  cancelRender = false;
  frameID++;
  // TODO: Maybe better as a kernel to avoid USM thrash to host
#ifndef OSPRAY_TARGET_SYCL
  getSh()->cancelRender = 0;
  getSh()->numPixelsRendered = 0;
  getSh()->frameID = frameID;
#endif
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
  getSh()->cancelRender = 1;
}

bool FrameBuffer::frameCancelled() const
{
  return cancelRender;
}

void FrameBuffer::prepareLiveOpsForFBV(
    FrameBufferView &fbv, bool fillFrameOps, bool fillPixelOps)
{
  // Iterate through all image operations set on commit
  for (auto &&obj : *imageOpData) {
    // Populate pixel operations
    PixelOp *pop = dynamic_cast<PixelOp *>(obj);
    if (pop) {
      if (fillPixelOps) {
        pixelOps.push_back(pop->attach());
        pixelOpShs.push_back(pixelOps.back()->getSh());
      }
    } else {
      // Populate frame operations
      FrameOpInterface *fopi = dynamic_cast<FrameOpInterface *>(obj);
      if (fillFrameOps && fopi)
        frameOps.push_back(fopi->attach(fbv));
    }
  }

  // Prepare shared parameters for kernel
  getSh()->pixelOps = pixelOpShs.empty() ? nullptr : pixelOpShs.data();
  getSh()->numPixelOps = pixelOpShs.size();
}

bool FrameBuffer::hasAccumBuf() const
{
  return hasAccumBuffer;
}

bool FrameBuffer::hasVarianceBuf() const
{
  return hasVarianceBuffer;
}

bool FrameBuffer::hasNormalBuf() const
{
  return hasNormalBuffer;
}

bool FrameBuffer::hasAlbedoBuf() const
{
  return hasAlbedoBuffer;
}

uint32 FrameBuffer::getChannelFlags() const
{
  uint32 channels = 0;
  if (hasDepthBuffer) {
    channels |= OSP_FB_DEPTH;
  }
  if (hasAccumBuffer) {
    channels |= OSP_FB_ACCUM;
  }
  if (hasVarianceBuffer) {
    channels |= OSP_FB_VARIANCE;
  }
  if (hasNormalBuffer) {
    channels |= OSP_FB_NORMAL;
  }
  if (hasAlbedoBuffer) {
    channels |= OSP_FB_ALBEDO;
  }
  return channels;
}

bool FrameBuffer::hasPrimitiveIDBuf() const
{
  return hasPrimitiveIDBuffer;
}

bool FrameBuffer::hasObjectIDBuf() const
{
  return hasObjectIDBuffer;
}

bool FrameBuffer::hasInstanceIDBuf() const
{
  return hasInstanceIDBuffer;
}

OSPTYPEFOR_DEFINITION(FrameBuffer *);

} // namespace ospray
