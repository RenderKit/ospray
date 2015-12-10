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

#include "LocalFB.ih"

// number of floats each task is clearing; must be a a mulitple of 16
#define CLEAR_BLOCK_SIZE (32 * 1024)

export void LocalFrameBuffer_clearAccum(void *uniform _fb)
{
  uniform LocalFB *uniform fb = (uniform LocalFB *uniform)_fb;
  fb->super.accumID = 0;

  if (fb->accumBuffer) {
    uniform size_t num_floats = 4*fb->super.size.x*fb->super.size.y;
    uniform size_t num_blocks = (num_floats + CLEAR_BLOCK_SIZE - 1) / CLEAR_BLOCK_SIZE;

    //NOTE(jda) - this needs to be threaded? seems to perform ok as-is...
    for(uniform int taskIndex = 0; taskIndex < num_blocks; ++taskIndex) {
      uniform float *uniform fbPointer
        = (uniform float *uniform)&fb->accumBuffer[0].x;
      uniform float *uniform block = fbPointer + taskIndex * CLEAR_BLOCK_SIZE;
      uniform size_t num_floats = 4*fb->super.size.x*fb->super.size.y;

      uniform int end = min(CLEAR_BLOCK_SIZE,
                            num_floats - taskIndex * CLEAR_BLOCK_SIZE);
      foreach (x=0 ... end)
        block[x] = 0.f;
    }
  }
}

//! \brief write tile into the given frame buffer's color buffer
/*! \detailed this buffer _must_ exist when this fct is called, and it
    _must_ have RGBA_I8 format */
export void LocalFrameBuffer_writeTile_RGBA_I8(void *uniform _fb,
                                               uniform Tile &tile)
{
  uniform LocalFB *uniform fb    = (uniform LocalFB *uniform)_fb;
  uniform uint32  *uniform color = (uniform uint32 *uniform)fb->colorBuffer;
  uniform float   *uniform depth = (uniform float *uniform)fb->depthBuffer;
  if (!color)
    // actually, this should never happen ...
    return;

  VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
  for (uniform uint32 iy=0;iy<TILE_SIZE;iy++) {
    uniform uint32 iiy=tile.region.lower.y+iy;
    if (iiy >= tile.region.upper.y) continue;

    uniform uint32 chunkID = iy*(TILE_SIZE/programCount);

    for (uint32 iix = tile.region.lower.x+programIndex;
         iix<tile.region.upper.x;iix+=programCount,chunkID++) {

      uint32 pixelID = iiy*fb->super.size.x+iix;
      unmasked {
        varying vec4f col = make_vec4f(varyTile->r[chunkID],
                                       varyTile->g[chunkID],
                                       varyTile->b[chunkID],
                                       varyTile->a[chunkID]);

        col = pow(max(col,make_vec4f(0.f)), 1.f/2.2f); // XXX hardcoded gamma, should use pixelops!

        const uint32 asRGBA = cvt_uint32(col);

        // uniform bool hasDepth = (fb->depthBuffer != NULL);
        // const uniform float accScale = 1.f/(fb->inherited.accumID+1);
        // if (fb->inherited.colorBufferFormat == ColorBufferFormat_RGBA_FLOAT32) {
        //   uniform vec4f *uniform color
        //     = fb->colorBuffer
        //     ? (uniform vec4f *uniform)fb->colorBuffer
        //     : NULL;
        //   uniform vec4f *uniform accum
        //     = fb->accumBuffer
        //     ? (uniform vec4f *uniform)fb->accumBuffer
        //     : NULL;
        //   uniform float *uniform depth
        //     = fb->depthBuffer
        //     ? (uniform float *uniform)fb->depthBuffer
        //     : NULL;
        //   for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
        //     const uint32 pixID = i + programIndex;
        //     const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
        //     const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
        //     const uint32  ofs   = y*fb->inherited.size.x+x;
        //     const vec4f value = getRGBA(tile,pixID);
        //     if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
        //       if (accum) {
        //         vec4f acc = accum[ofs]+value;
        //         accum[ofs] = acc;
        //         if (color) {
        //           color[ofs] = pow(max(acc * accScale,make_vec4f(0.f)), rcpf(fb->inherited.gamma));
        //         }
        //       } else
        //         if (color)
        //           color[ofs] = pow(max(value,make_vec4f(0.f)), rcpf(fb->inherited.gamma));
        //       if (depth)
        //         fb->depthBuffer[ofs] = tile.z[pixID];
        //     }
        //   }
        // } else if (fb->super.colorBufferFormat == ColorBufferFormat_RGBA_UINT8) {
        //   uniform uint32 *uniform color
        //     = fb->colorBuffer
        //     ? (uniform uint32 *uniform)fb->colorBuffer
        //     : NULL;
        //   uniform vec4f *uniform accum
        //     = fb->accumBuffer
        //     ? (uniform vec4f *uniform)fb->accumBuffer
        //     : NULL;
        //   uniform float *uniform depth
        //     = fb->depthBuffer
        //     ? (uniform float *uniform)fb->depthBuffer
        //     : NULL;


        //   for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
        //     const uint32 pixID = i + programIndex;
        //     const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
        //     const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
        //     const uint32  ofs   = y*fb->super.size.x+x;
        //     const vec4f value = getRGBA(tile,pixID);
        //     if (x < fb->super.size.x & y < fb->super.size.y) {
        //       if (accum) {
        //         vec4f acc = accum[ofs]+value;
        //         accum[ofs] = acc;
        //         if (color) {
        //           color[ofs] = cvt_uint32(pow(max(acc * accScale,make_vec4f(0.f)), rcpf(fb->super.gamma)));
        //         }
        //       } else
        //         if (color)
        //           color[ofs] = cvt_uint32(pow(max(value,make_vec4f(0.f)), rcpf(fb->super.gamma)));
        //       if (depth)
        //         fb->depthBuffer[ofs] = tile.z[pixID];
        //     }
      }
      color[pixelID] = asRGBA;
      if (depth)
        fb->depthBuffer[pixelID] = varyTile->z[chunkID];
    }
  }
}


//! \brief write tile into the given frame buffer's color buffer
/*! \detailed this buffer _must_ exist when this fct is called, and it
    _must_ have RGBA_F32 format */
export void LocalFrameBuffer_writeTile_RGBA_F32(void *uniform _fb,
                                                uniform Tile &tile)
{
  uniform LocalFB *uniform fb    = (uniform LocalFB *uniform)_fb;
  uniform vec4f   *uniform color = (uniform vec4f *uniform)fb->colorBuffer;
  uniform float   *uniform depth = (uniform float *uniform)fb->depthBuffer;
  if (!color)
    // actually, this should never happen ...
    return;

  VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
  for (uniform uint32 iy=0;iy<TILE_SIZE;iy++) {
    uniform uint32 iiy=tile.region.lower.y+iy;
    if (iiy >= tile.region.upper.y) continue;

    uniform uint32 chunkID = iy*(TILE_SIZE/programCount);

    for (uint32 iix = tile.region.lower.x+programIndex;
         iix<tile.region.upper.x;iix+=programCount,chunkID++) {

      uint32 pixelID = iiy*fb->super.size.x+iix;
      unmasked {
        varying vec4f col = make_vec4f(varyTile->r[chunkID],
                                       varyTile->g[chunkID],
                                       varyTile->b[chunkID],
                                       varyTile->a[chunkID]);

        col = pow(max(col,make_vec4f(0.f)), 1.f/2.2f); // XXX hardcoded gamma, should use pixelops!
      }
      color[pixelID] = col;
      if (depth)
        fb->depthBuffer[pixelID] = varyTile->z[chunkID];
    }
  }
}


//! \brief accumulate tile into BOTH accum buffer AND tile.
/*! \detailed After this call, the frame buffer will contain 'prev
    accum value + tile value', while the tile will contain '(prev
    accum value + tile value/numAccums' */
export void LocalFrameBuffer_accumulateTile(void *uniform _fb,
                                            uniform Tile &tile)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform vec4f *uniform accum = (uniform vec4f *uniform)fb->accumBuffer;
  if (!accum) return;

  const float accScale = rcpf(fb->super.accumID+1);

  VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
  for (uniform uint32 iy=0;iy<TILE_SIZE;iy++) {
    uniform uint32 iiy=tile.region.lower.y+iy;
    if (iiy >= tile.region.upper.y) continue;

    uniform uint32 chunkID = iy*(TILE_SIZE/programCount);

    for (uint32 iix = tile.region.lower.x+programIndex;
         iix<tile.region.upper.x;iix+=programCount,chunkID++) {

      uint32 pixelID = iiy*fb->super.size.x+iix;

      /*! todo: rather than gathering, replace this code with
          'load4f's and swizzles */
      varying vec4f acc = make_vec4f(0.f);
      if (fb->super.accumID > 0)
        acc = accum[pixelID];
      unmasked {
        acc.x += varyTile->r[chunkID];
        acc.y += varyTile->g[chunkID];
        acc.z += varyTile->b[chunkID];
        acc.w += varyTile->a[chunkID];
      }

      accum[pixelID] = acc;

      unmasked {
        varyTile->r[chunkID] = accScale * acc.x;
        varyTile->g[chunkID] = accScale * acc.y;
        varyTile->b[chunkID] = accScale * acc.z;
        varyTile->a[chunkID] = accScale * acc.w;
      }
    }
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
  uniform LocalFB *uniform self = uniform new uniform LocalFB;
  FrameBuffer_Constructor(&self->super,cClassPtr);
  FrameBuffer_set(&self->super,size_x,size_y,colorBufferFormat);

  self->colorBuffer = colorBuffer;
  self->depthBuffer = (uniform float *uniform)depthBuffer;
  self->accumBuffer = (uniform vec4f *uniform)accumBuffer;
  return self;
  // uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  // fb->super.setTile    = LocalFrameBuffer_setTile;
  // fb->super.accumTile  = LocalFrameBuffer_accumTile;
  // fb->super.accumID    = -1;
  // fb->super.size.x     = size_x;
  // fb->super.size.y     = size_y;
  // fb->super.rcpSize.x  = 1.f/size_x;
  // fb->super.rcpSize.y  = 1.f/size_y;
  // fb->super.gamma = 1.0f;
  // fb->colorBuffer = colorBuffer;
  // fb->depthBuffer = (uniform float *uniform)depthBuffer;
  // fb->accumBuffer = (uniform vec4f *uniform)accumBuffer;
  // fb->super.colorBufferFormat
  //   = (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
  // return fb;
  // uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  // fb->inherited.setTile    = LocalFrameBuffer_setTile;
  // fb->inherited.accumTile  = LocalFrameBuffer_accumTile;
  // fb->inherited.accumID    = -1;
  // fb->inherited.size.x     = size_x;
  // fb->inherited.size.y     = size_y;
  // fb->inherited.rcpSize.x  = 1.f/size_x;
  // fb->inherited.rcpSize.y  = 1.f/size_y;
  // fb->inherited.rcpSize_1.x  = 1.f/(size_x-1.f);
  // fb->inherited.rcpSize_1.y  = 1.f/(size_y-1.f);
  // fb->inherited.gamma = 1.0f;
  // fb->colorBuffer = colorBuffer;
  // fb->depthBuffer = (uniform float *uniform)depthBuffer;
  // fb->accumBuffer = (uniform vec4f *uniform)accumBuffer;
  // fb->inherited.colorBufferFormat
  //   = (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
  // return fb;
}
