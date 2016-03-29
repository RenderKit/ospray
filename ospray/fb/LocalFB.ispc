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

#include "LocalFB.ih"

// number of floats each task is clearing; must be a mulitple of 16
// NOTE(jda) - must match CLEAR_BLOCK_SIZE defined in LocalFB.cpp!
#define CLEAR_BLOCK_SIZE (32 * 1024)

export void LocalFrameBuffer_clearAccum(void *uniform _fb,
                                        uniform int taskIndex)
{
  uniform LocalFB *uniform fb = (uniform LocalFB *uniform)_fb;

  if (fb->accumBuffer) {
    uniform float *uniform fbPointer
      = (uniform float *uniform)&fb->accumBuffer[0].x;
    uniform float *uniform block = fbPointer + taskIndex * CLEAR_BLOCK_SIZE;
    uniform size_t num_floats = 4*fb->super.size.x*fb->super.size.y;

    uniform int end = min(CLEAR_BLOCK_SIZE,
                          num_floats - (taskIndex * CLEAR_BLOCK_SIZE));
    foreach (x=0 ... end) {
      block[x] = 0.f;
    }
  }
}

export void LocalFrameBuffer_clearVariance(void *uniform _fb, uniform int taskIndex)
{
  uniform LocalFB *uniform fb = (uniform LocalFB *uniform)_fb;

  if (fb->varianceBuffer) {
    uniform float *uniform fbPointer = (uniform float *uniform)&fb->varianceBuffer[0].x;
    uniform float *uniform block = fbPointer + taskIndex * CLEAR_BLOCK_SIZE;
    uniform size_t num_floats = 4*fb->super.size.x*fb->super.size.y;
    uniform int end = min(CLEAR_BLOCK_SIZE, num_floats - taskIndex * CLEAR_BLOCK_SIZE);

    foreach (x=0 ... end)
      block[x] = 0.f;
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

        // XXX hardcoded gamma, should use pixelops!
        col = max(col,make_vec4f(0.f));
        // alpha is never gamma-corrected
        col = make_vec4f(pow(make_vec3f(col), 1.f/2.2f), col.w);

        const uint32 asRGBA = cvt_uint32(col);
      }
      color[pixelID] = asRGBA;
      if (depth) fb->depthBuffer[pixelID] = varyTile->z[chunkID];
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

        // XXX not even hardcoded gamma for float buffer,
        // should use pixelops anyway
        col = max(col, make_vec4f(0.f));
      }
      color[pixelID] = col;
      if (depth) fb->depthBuffer[pixelID] = varyTile->z[chunkID];
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
  uniform vec4f *uniform accum = fb->accumBuffer;
  if (!accum) return;

  VaryingTile *uniform varyTile = (VaryingTile *uniform)&tile;
  uniform vec2i tileIdx = tile.region.lower/TILE_SIZE;

  uniform vec4f *uniform variance = fb->varianceBuffer;
  const uniform float accScale = rcpf(tile.accumID+1);
  const uniform float accHalfScale = rcpf(tile.accumID/2+1);
  float err = 0.f;
  float cnt = 0.f;

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
      if (tile.accumID > 0)
        acc = accum[pixelID];
      unmasked {
        acc.x += varyTile->r[chunkID];
        acc.y += varyTile->g[chunkID];
        acc.z += varyTile->b[chunkID];
        acc.w += varyTile->a[chunkID];
      }

      accum[pixelID] = acc;

      if (variance) {
        varying vec3f vari = make_vec3f(0.f);
        if (tile.accumID > 0)
          vari = make_vec3f(variance[pixelID]);
        if ((tile.accumID & 1) == 0) {
          unmasked {
            vari.x += varyTile->r[chunkID];
            vari.y += varyTile->g[chunkID];
            vari.z += varyTile->b[chunkID];
          }

          variance[pixelID] = make_vec4f(vari);
        }
        const vec3f accs = accScale * make_vec3f(acc);
        const vec3f diff = absf(accs - accHalfScale * vari);
        const float den = sqrtf(accs.x + accs.y + accs.z);
        if (den > 0)
          err = err + (diff.x + diff.y + diff.z) / den;
        cnt += 1.0f;
      }

      unmasked {
        varyTile->r[chunkID] = accScale * acc.x;
        varyTile->g[chunkID] = accScale * acc.y;
        varyTile->b[chunkID] = accScale * acc.z;
        varyTile->a[chunkID] = accScale * acc.w;
      }
    }
  }

  const uniform tileId = tileIdx.y*fb->numTiles.x + tileIdx.x;
  fb->tileAccumID[tileId]++;
  if (variance) {
    const uniform float r = sqrt(reduce_add(cnt) / tile.fbSize.x / tile.fbSize.y);
    const uniform float errf = r * reduce_add(err) / reduce_add(cnt);
    // print("[%, %]:  \t%\t%\n", tileIdx.x, tileIdx.y, errf);
    fb->tileErrorBuffer[tileId] = errf;
  }
}


export void *uniform LocalFrameBuffer_create(void *uniform cClassPtr,
                                             const uniform uint32 size_x,
                                             const uniform uint32 size_y,
                                             uniform int32 colorBufferFormat,
                                             void *uniform colorBuffer,
                                             void *uniform depthBuffer,
                                             void *uniform accumBuffer,
                                             void *uniform varianceBuffer,
                                             void *uniform tileAccumID,
                                             void *uniform tileErrorBuffer)
{
  uniform LocalFB *uniform self = uniform new uniform LocalFB;
  FrameBuffer_Constructor(&self->super,cClassPtr);
  FrameBuffer_set(&self->super,size_x,size_y,colorBufferFormat);

  self->colorBuffer = colorBuffer;
  self->depthBuffer = (uniform float *uniform)depthBuffer;
  self->accumBuffer = (uniform vec4f *uniform)accumBuffer;
  self->varianceBuffer = (uniform vec4f *uniform)varianceBuffer;
  self->numTiles = (self->super.size+(TILE_SIZE-1))/TILE_SIZE;
  self->tileAccumID = (uniform int32 *uniform)tileAccumID;
  self->tileErrorBuffer = (uniform float *uniform)tileErrorBuffer;
  return self;
}
