// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"
#include "ISPCDevice_ispc.h"
#include "OSPConfig.h"

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

FrameBuffer::FrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels)
    : size(_size),
      hasDepthBuffer(channels & OSP_FB_DEPTH),
      hasAccumBuffer(channels & OSP_FB_ACCUM),
      hasVarianceBuffer(
          (channels & OSP_FB_VARIANCE) && (channels & OSP_FB_ACCUM)),
      hasNormalBuffer(channels & OSP_FB_NORMAL),
      hasAlbedoBuffer(channels & OSP_FB_ALBEDO),
      hasPrimitiveIDBuffer(channels & OSP_FB_ID_PRIMITIVE),
      hasObjectIDBuffer(channels & OSP_FB_ID_OBJECT),
      hasInstanceIDBuffer(channels & OSP_FB_ID_INSTANCE)
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
  getSh()->renderTaskSize = renderTaskSize;
}

void FrameBuffer::commit()
{
  imageOpData = getParamDataT<ImageOp *>("imageOperation");
}

vec2i FrameBuffer::getRenderTaskSize() const
{
  return getSh()->renderTaskSize;
}

vec2i FrameBuffer::getNumPixels() const
{
  return getSh()->size;
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
  getSh()->cancelRender = 0;
  getSh()->numPixelsRendered = 0;
  getSh()->frameID++;
}

std::string FrameBuffer::toString() const
{
  return "ospray::FrameBuffer";
}

void FrameBuffer::setCompletedEvent(OSPSyncEvent event)
{
  // We won't be running ISPC-side rendering tasks when updating the
  // progress values here in C++
  if (event == OSP_NONE_FINISHED)
    getSh()->numPixelsRendered = 0;
  if (event == OSP_FRAME_FINISHED)
    getSh()->numPixelsRendered = getNumPixels().long_product();
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
  return static_cast<float>(getSh()->numPixelsRendered)
      / getNumPixels().long_product();
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

void FrameBuffer::prepareImageOps()
{
  findFirstFrameOperation();
  setPixelOpShs();
}

void FrameBuffer::findFirstFrameOperation()
{
  firstFrameOperation = -1;
  if (imageOps.empty())
    return;

  firstFrameOperation = imageOps.size();
  for (size_t i = 0; i < imageOps.size(); ++i) {
    const auto *obj = imageOps[i].get();
    const bool isFrameOp = dynamic_cast<const LiveFrameOp *>(obj) != nullptr;

    if (firstFrameOperation == imageOps.size() && isFrameOp)
      firstFrameOperation = i;
    else if (firstFrameOperation < imageOps.size() && !isFrameOp) {
      postStatusMsg(OSP_LOG_WARNING)
          << "Invalid pixel/frame op pipeline: all frame operations "
             "must come after all pixel operations";
    }
  }
}

void FrameBuffer::setPixelOpShs()
{
  pixelOpShs.clear();
  for (auto &op : imageOps) {
    LivePixelOp *pop = dynamic_cast<LivePixelOp *>(op.get());
    if (pop) {
      pixelOpShs.push_back(pop->getSh());
    }
  }
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

int32 FrameBuffer::getFrameID() const
{
  return getSh()->frameID;
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
