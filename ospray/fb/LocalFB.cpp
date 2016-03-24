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
#include "ospray/common/tasking/parallel_for.h"

// number of floats each task is clearing; must be a a mulitple of 16
// NOTE(jda) - must match CLEAR_BLOCK_SIZE defined in LocalFB.ispc!
#define CLEAR_BLOCK_SIZE (32 * 1024)

namespace ospray {

  LocalFrameBuffer::LocalFrameBuffer(const vec2i &size,
                                     ColorBufferFormat colorBufferFormat,
                                     bool hasDepthBuffer,
                                     bool hasAccumBuffer, 
                                     void *colorBufferToUse)
    : FrameBuffer(size, colorBufferFormat, hasDepthBuffer, hasAccumBuffer)
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
    ispcEquivalent = ispc::LocalFrameBuffer_create(this,size.x,size.y,
                                                   colorBufferFormat,
                                                   colorBuffer,
                                                   depthBuffer,
                                                   accumBuffer);
  }
  
  LocalFrameBuffer::~LocalFrameBuffer() 
  {
    if (depthBuffer) alignedFree(depthBuffer);

    if (colorBuffer)
      switch(colorBufferFormat) {
      case OSP_RGBA_F32:
        alignedFree(colorBuffer);
        break;
      case OSP_RGBA_I8:
        alignedFree(colorBuffer);
        break;
      default:
        throw std::runtime_error("color buffer format not supported");
      }
    if (accumBuffer) alignedFree(accumBuffer);
  }

  void LocalFrameBuffer::clear(const uint32 fbChannelFlags)
  {
    if (fbChannelFlags & OSP_FB_ACCUM) {
      void *thisIE = getIE();
      const int num_floats = 4 * size.x * size.y;
      const int num_blocks = (num_floats + CLEAR_BLOCK_SIZE - 1)
                             / CLEAR_BLOCK_SIZE;
      parallel_for(num_blocks,[&](int taskIndex) {
        ispc::LocalFrameBuffer_clearAccum(thisIE, taskIndex);
      });
      accumID = 0;
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
