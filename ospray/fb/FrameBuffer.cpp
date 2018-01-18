// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

  FrameBuffer::FrameBuffer(const vec2i &size,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer,
                           bool hasVarianceBuffer)
    : size(size),
      numTiles(divRoundUp(size, getTileSize())),
      maxValidPixelID(size-vec2i(1)),
      hasDepthBuffer(hasDepthBuffer),
      hasAccumBuffer(hasAccumBuffer),
      hasVarianceBuffer(hasVarianceBuffer),
      colorBufferFormat(colorBufferFormat),
      frameID(-1)
  {
    managedObjectType = OSP_FRAMEBUFFER;
    Assert(size.x > 0 && size.y > 0);
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

  void FrameBuffer::beginFrame()
  {
    frameID++;
    ispc::FrameBuffer_set_frameID(getIE(), frameID);
  }

  std::string FrameBuffer::toString() const
  {
    return "ospray::FrameBuffer";
  }
} // ::ospray
