// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"

namespace ospray {

FrameBuffer::FrameBuffer(const vec2i &_size,
    ColorBufferFormat _colorBufferFormat,
    const uint32 channels)
    : numTiles(divRoundUp(_size, getTileSize())),
      maxValidPixelID(_size - vec2i(1)),
      hasDepthBuffer(channels & OSP_FB_DEPTH),
      hasAccumBuffer(channels & OSP_FB_ACCUM),
      hasVarianceBuffer(
          (channels & OSP_FB_VARIANCE) && (channels & OSP_FB_ACCUM)),
      hasNormalBuffer(channels & OSP_FB_NORMAL),
      hasAlbedoBuffer(channels & OSP_FB_ALBEDO)
{
  managedObjectType = OSP_FRAMEBUFFER;
  if (_size.x <= 0 || _size.y <= 0) {
    throw std::runtime_error(
        "framebuffer has invalid size. Dimensions must be greater than 0");
  }
  getSh()->colorBufferFormat = _colorBufferFormat;
  getSh()->size = _size;
  getSh()->rcpSize = vec2f(1.f) / vec2f(_size);

  tileIDs.reserve(getTotalTiles());
  for (int i = 0; i < getTotalTiles(); ++i) {
    tileIDs.push_back(i);
  }
}

void FrameBuffer::commit()
{
  imageOpData = getParamDataT<ImageOp *>("imageOperation");
}

vec2i FrameBuffer::getTileSize() const
{
  return vec2i(TILE_SIZE);
}

vec2i FrameBuffer::getNumTiles() const
{
  return numTiles;
}

int FrameBuffer::getTotalTiles() const
{
  return numTiles.x * numTiles.y;
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

utility::ArrayView<int> FrameBuffer::getTileIDs()
{
  return utility::ArrayView<int>(tileIDs);
}

void FrameBuffer::beginFrame()
{
  frameProgress = 0.0f;
  cancelRender = false;
  getSh()->frameID++;
}

std::string FrameBuffer::toString() const
{
  return "ospray::FrameBuffer";
}

void FrameBuffer::setCompletedEvent(OSPSyncEvent event)
{
  if (event == OSP_NONE_FINISHED)
    frameProgress = 0.0f;
  if (event == OSP_FRAME_FINISHED)
    frameProgress = 1.0f;
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

void FrameBuffer::reportProgress(float newValue)
{
  frameProgress = clamp(newValue, 0.f, 1.f);
}

float FrameBuffer::getCurrentProgress()
{
  return frameProgress;
}

void FrameBuffer::cancelFrame()
{
  cancelRender = true;
}

bool FrameBuffer::frameCancelled() const
{
  return cancelRender;
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
      throw std::runtime_error(
          "frame operation must be before pixel and tile operations in the "
          "image operation pipeline");
    }
  }
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

OSPTYPEFOR_DEFINITION(FrameBuffer *);

} // namespace ospray
