// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

//ospray
#include "LocalFB.h"
#include "LocalFB_ispc.h"

namespace ospray {

  LocalFrameBuffer::LocalFrameBuffer(const vec2i &size,
                                     ColorBufferFormat colorBufferFormat,
                                     bool hasDepthBuffer,
                                     bool hasAccumBuffer,
                                     bool hasVarianceBuffer,
                                     void *colorBufferToUse)
    : FrameBuffer(size, colorBufferFormat, hasDepthBuffer,
                  hasAccumBuffer, hasVarianceBuffer)
      , tileErrorRegion(hasVarianceBuffer ? getNumTiles() : vec2i(0))
  {
    Assert(size.x > 0);
    Assert(size.y > 0);
    if (colorBufferToUse)
      colorBuffer = colorBufferToUse;
    else {
      switch (colorBufferFormat) {
      case OSP_FB_NONE:
        colorBuffer = nullptr;
        break;
      case OSP_FB_RGBA8:
      case OSP_FB_SRGBA:
        colorBuffer = (uint32*)alignedMalloc(sizeof(uint32)*size.x*size.y);
        break;
      case OSP_FB_RGBA32F:
        colorBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
        break;
      }
    }

    depthBuffer = hasDepthBuffer  ?
                  (float*)alignedMalloc(sizeof(float)*size.x*size.y) :
                  nullptr;

    accumBuffer = hasAccumBuffer ?
                  (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y) :
                  nullptr;

    const size_t bytes = sizeof(int32)*getTotalTiles();
    tileAccumID = (int32*)alignedMalloc(bytes);
    memset(tileAccumID, 0, bytes);

    varianceBuffer = hasVarianceBuffer ?
                     (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y) :
                     nullptr;

    ispcEquivalent = ispc::LocalFrameBuffer_create(this,size.x,size.y,
                                                   colorBufferFormat,
                                                   colorBuffer,
                                                   depthBuffer,
                                                   accumBuffer,
                                                   varianceBuffer,
                                                   tileAccumID);
  }

  LocalFrameBuffer::~LocalFrameBuffer()
  {
    alignedFree(depthBuffer);
    alignedFree(colorBuffer);
    alignedFree(accumBuffer);
    alignedFree(varianceBuffer);
    alignedFree(tileAccumID);
  }

  std::string LocalFrameBuffer::toString() const
  {
    return "ospray::LocalFrameBuffer";
  }

  void LocalFrameBuffer::clear(const uint32 fbChannelFlags)
  {
    frameID = -1; // we increment at the start of the frame
    if (fbChannelFlags & OSP_FB_ACCUM) {
      // it is only necessary to reset the accumID,
      // LocalFrameBuffer_accumulateTile takes care of clearing the
      // accumulation buffers
      memset(tileAccumID, 0, getTotalTiles()*sizeof(int32));

      // always also clear error buffer (if present)
      if (hasVarianceBuffer) {
        tileErrorRegion.clear();
      }
    }
  }

  void LocalFrameBuffer::setTile(Tile &tile)
  {
    if (pixelOp)
      pixelOp->preAccum(tile);
    if (accumBuffer) {
      const float err = ispc::LocalFrameBuffer_accumulateTile(getIE(),(ispc::Tile&)tile);
      if ((tile.accumID & 1) == 1)
        tileErrorRegion.update(tile.region.lower/TILE_SIZE, err);
    }
    if (pixelOp)
      pixelOp->postAccum(tile);
    if (colorBuffer) {
      switch (colorBufferFormat) {
      case OSP_FB_RGBA8:
        ispc::LocalFrameBuffer_writeTile_RGBA8(getIE(),(ispc::Tile&)tile);
        break;
      case OSP_FB_SRGBA:
        ispc::LocalFrameBuffer_writeTile_SRGBA(getIE(),(ispc::Tile&)tile);
        break;
      case OSP_FB_RGBA32F:
        ispc::LocalFrameBuffer_writeTile_RGBA32F(getIE(),(ispc::Tile&)tile);
        break;
      default:
        NOTIMPLEMENTED;
      }
    }
  }

  int32 LocalFrameBuffer::accumID(const vec2i &tile)
  {
    return tileAccumID[tile.y * numTiles.x + tile.x];
  }

  float LocalFrameBuffer::tileError(const vec2i &tile)
  {
    return tileErrorRegion[tile];
  }

  void LocalFrameBuffer::beginFrame()
  {
    FrameBuffer::beginFrame();
    if (pixelOp)
      pixelOp->beginFrame();
  }

  float LocalFrameBuffer::endFrame(const float errorThreshold)
  {
    if (pixelOp)
      pixelOp->endFrame();
    return tileErrorRegion.refine(errorThreshold);
  }

  const void *LocalFrameBuffer::mapDepthBuffer()
  {
    this->refInc();
    return (const void *)depthBuffer;
  }

  const void *LocalFrameBuffer::mapColorBuffer()
  {
    this->refInc();
    return (const void *)colorBuffer;
  }

  void LocalFrameBuffer::unmap(const void *mappedMem)
  {
    if (!(mappedMem == colorBuffer || mappedMem == depthBuffer)) {
      throw std::runtime_error("ERROR: unmapping a pointer not created by "
                               "OSPRay!");
    }
    this->refDec();
  }

} // ::ospray
