// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
  {
    Assert(size.x > 0);
    Assert(size.y > 0);
    if (colorBufferToUse)
      colorBuffer = colorBufferToUse;
    else {
      switch (colorBufferFormat) {
      case OSP_FB_NONE:
        colorBuffer = NULL;
        break;
      case OSP_FB_RGBA8:
      case OSP_FB_SRGBA:
        colorBuffer = (uint32*)alignedMalloc(sizeof(uint32)*size.x*size.y);
        break;
      case OSP_FB_RGBA32F:
        colorBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
        break;
      default:
        throw std::runtime_error("color buffer format not supported");
      }
    }

    if (hasDepthBuffer)
      depthBuffer = (float*)alignedMalloc(sizeof(float)*size.x*size.y);
    else
      depthBuffer = NULL;

    if (hasAccumBuffer)
      accumBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
    else
      accumBuffer = NULL;

    tilesx = divRoundUp(size.x, TILE_SIZE);
    tiles = tilesx * divRoundUp(size.y, TILE_SIZE);
    tileAccumID = (int32*)alignedMalloc(sizeof(int32)*tiles);
    memset(tileAccumID, 0, tiles*sizeof(int32));

    if (hasVarianceBuffer) {
      varianceBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
      tileErrorBuffer = (float*)alignedMalloc(sizeof(float)*tiles);
      // maximum number of regions: all regions are of size 3 are split in half
      errorRegion.reserve(divRoundUp(tiles*2, 3));
    } else {
      varianceBuffer = NULL;
      tileErrorBuffer = NULL;
    }

    ispcEquivalent = ispc::LocalFrameBuffer_create(this,size.x,size.y,
                                                   colorBufferFormat,
                                                   colorBuffer,
                                                   depthBuffer,
                                                   accumBuffer,
                                                   varianceBuffer,
                                                   tileAccumID,
                                                   tileErrorBuffer);
  }

  LocalFrameBuffer::~LocalFrameBuffer()
  {
    alignedFree(depthBuffer);
    alignedFree(colorBuffer);
    alignedFree(accumBuffer);
    alignedFree(varianceBuffer);
    alignedFree(tileAccumID);
    alignedFree(tileErrorBuffer);
  }

  std::string LocalFrameBuffer::toString() const
  {
    return "ospray::LocalFrameBuffer";
  }

  void LocalFrameBuffer::clear(const uint32 fbChannelFlags)
  {
    if (fbChannelFlags & OSP_FB_ACCUM) {
      // it is only necessary to reset the accumID,
      // LocalFrameBuffer_accumulateTile takes care of clearing the
      // accumulation buffers
      memset(tileAccumID, 0, tiles*sizeof(int32));

      // always also clear error buffer (if present)
      if (hasVarianceBuffer) {
        for (int i = 0; i < tiles; i++)
          tileErrorBuffer[i] = inf;

        errorRegion.clear();
        // initially create one region covering the complete image
        errorRegion.push_back(box2i(vec2i(0),
                                    vec2i(tilesx,
                                          divRoundUp(size.y, TILE_SIZE))));
      }
    }
  }

  void LocalFrameBuffer::setTile(Tile &tile)
  {
    if (pixelOp)
      pixelOp->preAccum(tile);
    if (accumBuffer)
      ispc::LocalFrameBuffer_accumulateTile(getIE(),(ispc::Tile&)tile);
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
    return tileAccumID[tile.y * tilesx + tile.x];
  }

  float LocalFrameBuffer::tileError(const vec2i &tile)
  {
    const int idx = tile.y * tilesx + tile.x;
    return hasVarianceBuffer ? tileErrorBuffer[idx] : inf;
  }

  float LocalFrameBuffer::endFrame(const float errorThreshold)
  {
    if (hasVarianceBuffer) {
      // process regions first, but don't process newly split regions again
      int regions = errorThreshold > 0.f ? errorRegion.size() : 0;
      for (int i = 0; i < regions; i++) {
        box2i& region = errorRegion[i];
        float err = 0.f;
        float maxErr = 0.0f;
        for (int y = region.lower.y; y < region.upper.y; y++)
          for (int x = region.lower.x; x < region.upper.x; x++) {
            int idx = y * tilesx + x;
            err += tileErrorBuffer[idx];
            maxErr = std::max(maxErr, tileErrorBuffer[idx]);
          }
        // set all tiles of this region to local max error to enforce their
        // refinement as a group
        for (int y = region.lower.y; y < region.upper.y; y++)
          for (int x = region.lower.x; x < region.upper.x; x++) {
            int idx = y * tilesx + x;
            tileErrorBuffer[idx] = maxErr;
          }
        vec2i size = region.size();
        int area = reduce_mul(size);
        err /= area; // avg
        if (err < 4.f*errorThreshold) { // split region?
          if (area <= 2) { // would just contain single tile after split: remove
            regions--;
            errorRegion[i] = errorRegion[regions];
            errorRegion[regions]= errorRegion.back();
            errorRegion.pop_back();
            i--;
            continue;
          }
          vec2i split = region.lower + size / 2; // TODO: find split with equal
                                                 //       variance
          errorRegion.push_back(region); // region ref might become invalid
          if (size.x > size.y) {
            errorRegion[i].upper.x = split.x;
            errorRegion.back().lower.x = split.x;
          } else {
            errorRegion[i].upper.y = split.y;
            errorRegion.back().lower.y = split.y;
          }
        }
      }

      float maxErr = 0.0f;
      for (int i = 0; i < tiles; i++)
        maxErr = std::max(maxErr, tileErrorBuffer[i]);

      return maxErr;
    } else
      return inf;
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
    Assert(mappedMem == colorBuffer || mappedMem == depthBuffer );
    this->refDec();
  }

} // ::ospray
