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

      uint32 pixelID = iiy*fb->inherited.size.x+iix;
      unmasked {
        varying vec4f col = make_vec4f(varyTile->r[chunkID],
                                       varyTile->g[chunkID],
                                       varyTile->b[chunkID],
                                       varyTile->a[chunkID]);
      }

      color[pixelID] = cvt_uint32(col);
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
  
  const float accScale = rcpf(fb->inherited.accumID+1);

  VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
  for (uniform uint32 iy=0;iy<TILE_SIZE;iy++) {
    uniform uint32 iiy=tile.region.lower.y+iy;
    if (iiy >= tile.region.upper.y) continue;

    uniform uint32 chunkID = iy*(TILE_SIZE/programCount);
    
    for (uint32 iix = tile.region.lower.x+programIndex;
         iix<tile.region.upper.x;iix+=programCount,chunkID++) {

      uint32 pixelID = iiy*fb->inherited.size.x+iix;
      
      /*! todo: rather than gathering, replace this code with
          'load4f's and swizzles */
      varying vec4f acc = make_vec4f(0.f);
      if (fb->inherited.accumID > 0) 
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


/*! deprecated - this was once used in ispc-side 'setTile' function
    pointers; but this is since handled on C++ side, which calls more
    specialized functions */
export void LocalFrameBuffer_setTile(void *uniform _fb, //uniform FrameBuffer *uniform _fb,
                                     uniform Tile &tile)
{
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  uniform bool hasDepth = (fb->depthBuffer != NULL);
  const float accScale = 1.f/(fb->inherited.accumID+1);
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
      if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
        const vec4f value = getRGBA(tile,pixID);
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
          fb->depthBuffer[ofs] = tile.z[i];
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
    VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
    for (uniform int iy=0;iy<TILE_SIZE;iy++) {
      uniform uint32 y = tile.region.lower.y + iy;
      if (y >= fb->inherited.size.y) 
        continue;
      uniform iix = iy*TILE_SIZE / programCount;
      for (uniform int ix=0;ix<TILE_SIZE;ix+=programCount,iix++) {
        const uint32 x = tile.region.lower.x+ix+programIndex;
        if (x >= fb->inherited.size.x)
          continue;
        
        const uint32 fb_ofs   = y*fb->inherited.size.x + x;
        const uint32 tile_ofs = (iy*TILE_SIZE+ix)+programIndex;
        vec4f value = make_vec4f(varyTile->r[iix],
                                 varyTile->g[iix],
                                 varyTile->b[iix],
                                 varyTile->a[iix]);
        if (accum) {
          value = value + accum[fb_ofs];
          accum[fb_ofs] = value;
          if (color) 
            color[fb_ofs] = cvt_uint32(value * accScale);
        } else
          if (color)
            color[fb_ofs] = cvt_uint32(value);
        if (depth)
          fb->depthBuffer[fb_ofs] = tile.z[tile_ofs];
      }
    }
// #else
//     for (uniform int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
//       const uint32 pixID = i + programIndex;
//       const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
//       const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
//       const uint32  ofs   = y*fb->inherited.size.x+x;
//       if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
//         const vec4f value = getRGBA(tile,pixID);
//         if (accum) {
//           vec4f acc = accum[ofs]+value;
//           accum[ofs] = acc;

//           if (color) {
//             color[ofs] = cvt_uint32(acc * accScale);
//           }
//         } else
//           if (color)
//             color[ofs] = cvt_uint32(value);
//         if (depth)
//           fb->depthBuffer[ofs] = tile.z[i];
//       }
//     }
// #endif
  } else {
    print("format UNKNOWN\n");
  }
}

// void LocalFrameBuffer_accumTile(uniform FrameBuffer *uniform _fb,
//                                 uniform Tile &tile)
// {
//   print("accum tile % %, %\n",tile.region.lower.x,tile.region.lower.y,tile.r[0]);
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  
//   uniform vec4f *uniform dst = (uniform vec4f *uniform)fb->accumBuffer;
//   for (int i=0;i<TILE_SIZE*TILE_SIZE;i+=programCount) {
//     //const uint32  pixID = i*programCount+programIndex;
//     const uint32 pixID = i + programIndex;
//     const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
//     const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
//     const uint32  ofs   = y*fb->inherited.size.x+x;
//     const vec4f value = getRGBA(tile,pixID);
//     if (x < fb->inherited.size.x & y < fb->inherited.size.y) 
//       dst[ofs]        = value;
//   }
// }

export void *uniform LocalFrameBuffer_create(void *uniform cClassPtr,
                                             const uniform uint32 size_x,
                                             const uniform uint32 size_y,
                                             uniform int32 colorBufferFormat,
                                             void *uniform colorBuffer,
                                             void *uniform depthBuffer,
                                             void *uniform accumBuffer)
{
  uniform LocalFB *uniform fb = uniform new uniform LocalFB;
  // fb->inherited.setTile    = LocalFrameBuffer_setTile;
  // fb->inherited.accumTile  = LocalFrameBuffer_accumTile;
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

