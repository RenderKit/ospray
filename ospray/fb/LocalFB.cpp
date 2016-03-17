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

#include "LocalFB.h"
#include "LocalFB_ispc.h"

namespace ospray {

  LocalFrameBuffer::LocalFrameBuffer(const vec2i &size,
                                     ColorBufferFormat colorBufferFormat,
                                     bool hasDepthBuffer,
                                     bool hasAccumBuffer,
                                     bool hasVarianceBuffer,
                                     void *colorBufferToUse)
    : FrameBuffer(size, colorBufferFormat, hasDepthBuffer, hasAccumBuffer, hasVarianceBuffer)
  {
    Assert(size.x > 0);
    Assert(size.y > 0);
    if (colorBufferToUse)
      colorBuffer = colorBufferToUse;
    else {
      switch(colorBufferFormat) {
      case OSP_RGBA_NONE:
        colorBuffer = NULL;
        break;
      case OSP_RGBA_F32:
        colorBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
        break;
      case OSP_RGBA_I8:
        colorBuffer = (uint32*)alignedMalloc(sizeof(uint32)*size.x*size.y);
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

    if (hasVarianceBuffer) {
      varianceBuffer = (vec4f*)alignedMalloc(sizeof(vec4f)*size.x*size.y);
      tileErrorBuffer = new float[divRoundUp(size.x,TILE_SIZE)*divRoundUp(size.y,TILE_SIZE)];
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
                                                   tileErrorBuffer);
  }

  LocalFrameBuffer::~LocalFrameBuffer()
  {
    alignedFree(depthBuffer);
    alignedFree(colorBuffer);
    alignedFree(accumBuffer);
    alignedFree(varianceBuffer);
    delete[] tileErrorBuffer;
  }

  void LocalFrameBuffer::clear(const uint32 fbChannelFlags)
  {
    if (fbChannelFlags & OSP_FB_ACCUM) {
      ispc::LocalFrameBuffer_clearAccum(getIE());
      // always also clear variance buffer -- only clearing accumulation buffer
      // is meaningless
      ispc::LocalFrameBuffer_clearVariance(getIE());
      accumID = 0;
    }
  }

  void LocalFrameBuffer::setTile(Tile &tile)
  {
#if 1
    if (pixelOp)
      pixelOp->preAccum(tile);
    if (accumBuffer)
      ispc::LocalFrameBuffer_accumulateTile(getIE(),(ispc::Tile&)tile);
    if (pixelOp)
      pixelOp->postAccum(tile);
    if (colorBuffer) {
      switch(colorBufferFormat) {
      case OSP_RGBA_I8:
        ispc::LocalFrameBuffer_writeTile_RGBA_I8(getIE(),(ispc::Tile&)tile);
        break;
      case OSP_RGBA_F32:
        ispc::LocalFrameBuffer_writeTile_RGBA_F32(getIE(),(ispc::Tile&)tile);
        break;
      default:
        NOTIMPLEMENTED;
      }
    }
#else
    ispc::LocalFrameBuffer_setTile(getIE(),(ispc::Tile&)tile);
#endif
  }

  const void *LocalFrameBuffer::mapDepthBuffer()
  {
    // waitForRenderTaskToBeReady();
    this->refInc();
    return (const void *)depthBuffer;
  }

  const void *LocalFrameBuffer::mapColorBuffer()
  {
    // waitForRenderTaskToBeReady();
    this->refInc();
    return (const void *)colorBuffer;
  }

  void LocalFrameBuffer::unmap(const void *mappedMem)
  {
    Assert(mappedMem == colorBuffer || mappedMem == depthBuffer );
    this->refDec();
  }

} // ::ospray
