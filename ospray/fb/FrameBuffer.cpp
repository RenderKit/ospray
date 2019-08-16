// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "FrameBuffer.h"
#include "FrameBuffer_ispc.h"

namespace ospray {

  FrameBuffer::FrameBuffer(const vec2i &_size,
                           ColorBufferFormat _colorBufferFormat,
                           const uint32 channels)
    : size(_size),
      numTiles(divRoundUp(size, getTileSize())),
      maxValidPixelID(size-vec2i(1)),
      colorBufferFormat(_colorBufferFormat),
      hasDepthBuffer(channels & OSP_FB_DEPTH),
      hasAccumBuffer(channels & OSP_FB_ACCUM),
      hasVarianceBuffer((channels & OSP_FB_VARIANCE)
                        && (channels & OSP_FB_ACCUM)),
      hasNormalBuffer(channels & OSP_FB_NORMAL),
      hasAlbedoBuffer(channels & OSP_FB_ALBEDO),
      frameID(-1)
  {
    managedObjectType = OSP_FRAMEBUFFER;
    if (size.x <= 0 || size.y <= 0) {
      throw std::runtime_error(
          "framebuffer has invalid size. Dimensions must be greater than 0");
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
    return size;
  }

  float FrameBuffer::getVariance() const
  {
    return frameVariance;
  }

  void FrameBuffer::beginFrame()
  {
    cancelRender = false;
    frameID++;
    ispc::FrameBuffer_set_frameID(getIE(), frameID);
  }

  std::string FrameBuffer::toString() const
  {
    return "ospray::FrameBuffer";
  }

  void FrameBuffer::setCompletedEvent(OSPSyncEvent event)
  {
    stagesCompleted = event;
  }

  OSPSyncEvent FrameBuffer::getLatestCompleteEvent() const
  {
    return stagesCompleted;
  }

  void FrameBuffer::waitForEvent(OSPSyncEvent event) const
  {
    // TODO: condition variable to sleep calling thread instead of spinning?
    while (stagesCompleted < event);
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

} // ::ospray
