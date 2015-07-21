// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/fb/Tile.ih"
#include "ospray/fb/FrameBuffer.ih"
#include "ospray/render/util.ih"

struct LocalFB 
{
  FrameBuffer inherited; /*!< inherit all methods and members from 'base' class */
  void *colorBuffer;
  uniform float *depthBuffer;
  uniform vec4f *accumBuffer;
};

// number of floats each task is clearing; must be a a mulitple of 16
#define CLEAR_BLOCK_SIZE (32 * 1024)

task void LocalFrameBuffer_clearAccum_task(uniform LocalFB *uniform fb)
{
  uniform float *uniform fbPointer 
    = (uniform float *uniform)&fb->accumBuffer[0].x;
  uniform float *uniform block = fbPointer + taskIndex * CLEAR_BLOCK_SIZE;
  uniform size_t num_floats = 4*fb->inherited.size.x*fb->inherited.size.y;
  
  foreach (x=0 ... min(CLEAR_BLOCK_SIZE,num_floats-taskIndex*CLEAR_BLOCK_SIZE))
    block[x] = 0.f;
}

export void LocalFrameBuffer_clearAccum(void *uniform _fb)
{
  uniform LocalFB *uniform fb = (uniform LocalFB *uniform)_fb;
  fb->inherited.accumID = 0;

  if (fb->accumBuffer) {
    uniform size_t num_floats = 4*fb->inherited.size.x*fb->inherited.size.y;
    uniform size_t num_blocks = (num_floats + CLEAR_BLOCK_SIZE - 1) / CLEAR_BLOCK_SIZE;
    launch[num_blocks] LocalFrameBuffer_clearAccum_task(fb);
  }
}

void LocalFrameBuffer_setTile(uniform FrameBuffer *uniform _fb,
                              uniform Tile &tile)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform bool hasDepth = (fb->depthBuffer != NULL);
  const uniform float accScale = 1.f/(fb->inherited.accumID+1);
  if (fb->inherited.colorBufferFormat == ColorBufferFormat_RGBA_FLOAT32) {
    uniform vec4f *uniform color
      = fb->colorBuffer 
      ? (uniform vec4f *uniform)fb->colorBuffer
      : NULL;
    uniform vec4f *uniform accum
      = fb->accumBuffer
      ? (uniform vec4f *uniform)fb->accumBuffer
      : NULL;
    uniform float *uniform depth
      = fb->depthBuffer 
      ? (uniform float *uniform)fb->depthBuffer
      : NULL;
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
      const uint32 pixID = i + programIndex;
      const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
      const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
      const uint32  ofs   = y*fb->inherited.size.x+x;
      const vec4f value = getRGBA(tile,pixID);
      if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
        if (accum) {
          vec4f acc = accum[ofs]+value;
          accum[ofs] = acc;
          if (color) {
            color[ofs] = acc * accScale;
          }
        } else
          if (color)
            color[ofs] = value;
        if (depth)
          fb->depthBuffer[ofs] = tile.z[pixID];
      }
    }
  } else if (fb->inherited.colorBufferFormat == ColorBufferFormat_RGBA_UINT8) {
    uniform uint32 *uniform color
      = fb->colorBuffer 
      ? (uniform uint32 *uniform)fb->colorBuffer
      : NULL;
    uniform vec4f *uniform accum
      = fb->accumBuffer
      ? (uniform vec4f *uniform)fb->accumBuffer
      : NULL;
    uniform float *uniform depth
      = fb->depthBuffer 
      ? (uniform float *uniform)fb->depthBuffer
      : NULL;


    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
      const uint32 pixID = i + programIndex;
      const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
      const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
      const uint32  ofs   = y*fb->inherited.size.x+x;
      const vec4f value = getRGBA(tile,pixID);
      if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
        if (accum) {
          vec4f acc = accum[ofs]+value;
          accum[ofs] = acc;

          acc = acc * accScale;
          //acc = pow(max(acc,make_vec4f(0.f)), 1.f/2.2f); // XXX hardcoded gamma, should use pixelops!

          if (color) {
            color[ofs] = cvt_uint32(acc);
          }
        } else
          if (color)
            color[ofs] = cvt_uint32(value);
        if (depth)
          fb->depthBuffer[ofs] = tile.z[pixID];
      }
    }
  } else {
    print("format UNKNOWN\n");
  }
}

void LocalFrameBuffer_accumTile(uniform FrameBuffer *uniform _fb,
                                uniform Tile &tile)
{
  print("accum tile % %, %\n",tile.region.lower.x,tile.region.lower.y,tile.r[0]);
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  
  uniform vec4f *uniform dst = (uniform vec4f *uniform)fb->accumBuffer;
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
    const uint32 pixID = i + programIndex;
    const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
    const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
    const uint32  ofs   = y*fb->inherited.size.x+x;
    const vec4f value = getRGBA(tile,pixID);
    if (x < fb->inherited.size.x & y < fb->inherited.size.y) 
      dst[ofs]        = value;
  }
}

export void *uniform LocalFrameBuffer_create(void *uniform cClassPtr,
                                             const uniform uint32 size_x,
                                             const uniform uint32 size_y,
                                             uniform int32 colorBufferFormat,
                                             void *uniform colorBuffer,
                                             void *uniform depthBuffer,
                                             void *uniform accumBuffer)
{
  uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  fb->inherited.setTile    = LocalFrameBuffer_setTile;
  fb->inherited.accumTile  = LocalFrameBuffer_accumTile;
  fb->inherited.accumID    = -1;
  fb->inherited.size.x     = size_x;
  fb->inherited.size.y     = size_y;
  fb->inherited.rcpSize.x  = 1.f/size_x;
  fb->inherited.rcpSize.y  = 1.f/size_y;
  fb->colorBuffer = colorBuffer;
  fb->depthBuffer = (uniform float *uniform)depthBuffer;
  fb->accumBuffer = (uniform vec4f *uniform)accumBuffer;
  fb->inherited.colorBufferFormat
    = (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
  return fb;
}

