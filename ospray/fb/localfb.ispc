/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



#include "ospray/fb/tile.ih"
#include "ospray/fb/framebuffer.ih"
#include "ospray/render/util.ih"

struct LocalFB 
{
  FrameBuffer inherited; /*!< inherit all methods and members from 'base' class */
  void *colorBuffer;
  uniform float *depthBuffer;
  uniform vec4f *accumBuffer;
  
};

task void LocalFrameBuffer_clearAccum_task(uniform LocalFB *uniform fb)
{
  uniform float *uniform line
    = (uniform float *uniform)&fb->accumBuffer[taskIndex*fb->inherited.size.x].x;
  foreach (x=0 ... 4*fb->inherited.size.x)
    line[x] = 0.f;
}

export void LocalFrameBuffer_clearAccum(void *uniform _fb)
{
  uniform LocalFB *uniform fb = (uniform LocalFB *uniform)_fb;

  fb->inherited.accumID = 0;

  if (fb->accumBuffer)
    launch[fb->inherited.size.y] LocalFrameBuffer_clearAccum_task(fb);
}

// =======================================================
// float4 version of frame buffer 
// =======================================================
// void localfb_vec4f_destruct(uniform FrameBuffer *uniform fb)
// {
//   delete fb;
// }
// void localfb_vec4f_setPixel(uniform FrameBuffer *uniform _fb,
//                             const varying vec2i &coord,
//                             const varying vec4f &value)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
//   pixel[coord.x+coord.y*fb->inherited.size.x] = value;
// }

// vec4f localfb_vec4f_getPixel(uniform FrameBuffer *uniform _fb,
//                              const varying vec2i &coord)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
//   return pixel[coord.x+coord.y*fb->inherited.size.x];
// }

// export void *uniform ispc__createLocalFB_RGBA_F32(const uniform uint32 size_x,
//                                                   const uniform uint32 size_y,
//                                                   void *uniform cClassPtr,
//                                                   void *uniform pixelPtr)
// {
//   uniform LocalFB *uniform fb = uniform new uniform LocalFB;
//   fb->inherited.destructor = localfb_vec4f_destruct;
//   fb->inherited.setPixel   = localfb_vec4f_setPixel;
//   fb->inherited.getPixel   = localfb_vec4f_getPixel;
//   fb->inherited.cClassPtr  = cClassPtr;
//   fb->inherited.size.x     = size_x;
//   fb->inherited.size.y     = size_y;
//   fb->pixel = pixelPtr;
//   return fb;
// }

// // =======================================================
// // vec4uc version of frame buffer 
// // =======================================================
// inline float extractUcharChannel(uint32 pix, uniform const int32 shift)
// {
//   uint32 i = (pix >> shift) & 255;
//   return i * (1.f/255.f);
// }

// inline uint32 cvt_uint32(const float f)
// {
//   return (uint32)(255.9f * max(min(f,1.f),0.f));
// }

// inline uint32 cvt_uint32(const vec4f &v)
// {
//   return 
//     (cvt_uint32(v.x) << 0)  |
//     (cvt_uint32(v.y) << 8)  |
//     (cvt_uint32(v.z) << 16) |
//     (cvt_uint32(v.w) << 24);
// }

// void localfb_uint32_destruct(uniform FrameBuffer *uniform fb)
// {
//   delete fb;
// }

// void localfb_uint32_setPixel(uniform FrameBuffer *uniform _fb,
//                             const varying vec2i &coord,
//                             const varying vec4f &value)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;

//   const uint32 ofs = coord.x+coord.y*fb->inherited.size.x;
//   pixel[ofs] = cvt_uint32(value);
// }

void LocalFrameBuffer_setTile(uniform FrameBuffer *uniform _fb,
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
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      const uint32  pixID = i*programCount+programIndex;
      const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
      const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
      const uint32  ofs   = y*fb->inherited.size.x+x;
      const vec4f value = getRGBA(tile,i);
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


    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      const uint32  pixID = i*programCount+programIndex;
      const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
      const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
      const uint32  ofs   = y*fb->inherited.size.x+x;
      const vec4f value = getRGBA(tile,i);
      if (x < fb->inherited.size.x & y < fb->inherited.size.y) {
        //        dst[ofs]        = cvt_uint32(value);
        if (accum) {
          vec4f acc = accum[ofs]+value;
          accum[ofs] = acc;

          if (color) {
            color[ofs] = cvt_uint32(acc * accScale);
          }
        } else
          if (color)
            color[ofs] = cvt_uint32(value);
        if (depth)
          fb->depthBuffer[ofs] = tile.z[i];
      }
    }
  } else {
    print("format UNKONWN\n");
  }
}

void LocalFrameBuffer_accumTile(uniform FrameBuffer *uniform _fb,
                                uniform Tile &tile)
{
  print("accum tile % %, %\n",tile.region.lower.x,tile.region.lower.y,tile.r[0]);
  uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
  
  uniform vec4f *uniform dst = (uniform vec4f *uniform)fb->accumBuffer;
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32  pixID = i*programCount+programIndex;
    const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
    const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
    const uint32  ofs   = y*fb->inherited.size.x+x;
    const vec4f value = getRGBA(tile,i);
    if (x < fb->inherited.size.x & y < fb->inherited.size.y) 
      dst[ofs]        = value;
  }
}

// vec4f localfb_uint32_getPixel(uniform FrameBuffer *uniform _fb,
//                               const varying vec2i &coord)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
//   uint32 pix = pixel[coord.x+coord.y*fb->inherited.size.x];
//   return make_vec4f(extractUcharChannel(pix,0),
//                     extractUcharChannel(pix,8),
//                     extractUcharChannel(pix,16),
//                     extractUcharChannel(pix,24));
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
  fb->inherited.colorBufferFormat = (uniform FrameBuffer_ColorBufferFormat)colorBufferFormat;
  return fb;
}

// export void ispc__setTile(void *uniform _fb, void *uniform _tile)
// {
//   uniform Tile *uniform        tile = (uniform Tile *uniform)_tile;
//   uniform FrameBuffer *uniform fb   = (uniform FrameBuffer *uniform)_fb;
//   fb->setTile(fb,tile);
// }

// /*! \file localfb.ispc ISPC-side code for a local frame buffer */

// #include "framebuffer.ih"
// #include "tile.ih"

// struct LocalFB 
// {
//   FrameBuffer inherited; /*!< inherit all methods and members from 'base' class */
//   void *pixel;
//   void *accum;
// };

// // =======================================================
// // float4 version of frame buffer 
// // =======================================================
// void localfb_vec4f_destruct(uniform FrameBuffer *uniform fb)
// {
//   delete fb;
// }
// void localfb_vec4f_setPixel(uniform FrameBuffer *uniform _fb,
//                             const varying vec2i &coord,
//                             const varying vec4f &value)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
//   pixel[coord.x+coord.y*fb->inherited.size.x] = value;
// }

// vec4f localfb_vec4f_getPixel(uniform FrameBuffer *uniform _fb,
//                              const varying vec2i &coord)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform vec4f *uniform pixel = (uniform vec4f *uniform)fb->pixel;
//   return pixel[coord.x+coord.y*fb->inherited.size.x];
// }

// export void *uniform ispc__createLocalFB_RGBA_F32(const uniform uint32 size_x,
//                                                   const uniform uint32 size_y,
//                                                   void *uniform cClassPtr,
//                                                   void *uniform pixelPtr)
// {
//   uniform LocalFB *uniform fb = uniform new uniform LocalFB;
//   fb->inherited.destructor = localfb_vec4f_destruct;
//   fb->inherited.setPixel   = localfb_vec4f_setPixel;
//   fb->inherited.getPixel   = localfb_vec4f_getPixel;
//   fb->inherited.cClassPtr  = cClassPtr;
//   fb->inherited.size.x     = size_x;
//   fb->inherited.size.y     = size_y;
//   fb->pixel = pixelPtr;
//   return fb;
// }

// // =======================================================
// // vec4uc version of frame buffer 
// // =======================================================
// inline float extractUcharChannel(uint32 pix, uniform const int32 shift)
// {
//   uint32 i = (pix >> shift) & 255;
//   return i * (1.f/255.f);
// }

// inline uint32 cvt_uint32(const float f)
// {
//   return (uint32)(255.9f * max(min(f,1.f),0.f));
// }

// inline uint32 cvt_uint32(const vec4f &v)
// {
//   return 
//     (cvt_uint32(v.x) << 0)  |
//     (cvt_uint32(v.y) << 8)  |
//     (cvt_uint32(v.z) << 16) |
//     (cvt_uint32(v.w) << 24);
// }

// void localfb_uint32_destruct(uniform FrameBuffer *uniform fb)
// {
//   delete fb;
// }

// void localfb_uint32_setPixel(uniform FrameBuffer *uniform _fb,
//                             const varying vec2i &coord,
//                             const varying vec4f &value)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;

//   const uint32 ofs = coord.x+coord.y*fb->inherited.size.x;
//   pixel[ofs] = cvt_uint32(value);
// }

// void localfb_accumTile(uniform FrameBuffer *uniform _fb,
//                        uniform Tile *uniform tile)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform vec4f *uniform accum = (uniform vec4f *uniform)fb->accum;

//   for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
//     const uint32  pixID = i*programCount+programIndex;
//     const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
//     const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
//     const uint32  ofs   = y*tile.fbSize.x+x;
//     // const vec4f value = make_vec4f(tile.r[i],tile.g[i],tile.b[i],tile.a[i]);
//     if (x < tile.fbSize.x & y < tile.fbSize.y) 
//       accum[ofs]        = accum[ofs]+getRGBA(*tile,i);
//   }
// }

// void localfb_uint32_setTile(uniform FrameBuffer *uniform _fb,
//                             uniform Tile *uniform tile)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
//   for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
//     const uint32  pixID = i*programCount+programIndex;
//     const uint32  x     = tile.region.lower.x + (pixID % TILE_SIZE);
//     const uint32  y     = tile.region.lower.y + (pixID / TILE_SIZE);
//     const uint32  ofs   = y*tile.fbSize.x+x;
//     const vec4f value = make_vec4f(tile.r[i],tile.g[i],tile.b[i],tile.a[i]);
//     if (x < tile.fbSize.x & y < tile.fbSize.y) {
//       pixel[ofs]        = cvt_uint32(value);
//     }
//   }
// }

// vec4f localfb_uint32_getPixel(uniform FrameBuffer *uniform _fb,
//                               const varying vec2i &coord)
// {
//   uniform LocalFB *uniform fb  = (uniform LocalFB *uniform)_fb;
//   uniform uint32 *uniform pixel = (uniform uint32 *uniform)fb->pixel;
//   uint32 pix = pixel[coord.x+coord.y*fb->inherited.size.x];
//   return make_vec4f(extractUcharChannel(pix,0),
//                     extractUcharChannel(pix,8),
//                     extractUcharChannel(pix,16),
//                     extractUcharChannel(pix,24));
// }

// export void *uniform ispc__createLocalFB_RGBA_I8(const uniform uint32 size_x,
//                                                  const uniform uint32 size_y,
//                                                  void *uniform cClassPtr,
//                                                  void *uniform pixelPtr)
// {
//   uniform LocalFB *uniform fb = uniform new uniform LocalFB;
//   fb->inherited.destructor = localfb_uint32_destruct;
//   fb->inherited.setPixel   = localfb_uint32_setPixel;
//   fb->inherited.getPixel   = localfb_uint32_getPixel;
//   fb->inherited.setTile    = localfb_uint32_setTile;
//   fb->inherited.cClassPtr  = cClassPtr;
//   fb->inherited.size.x     = size_x;
//   fb->inherited.size.y     = size_y;
//   fb->pixel = pixelPtr;
//   return fb;
// }

