// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LocalFB.ih"

//! \brief write tile into the given frame buffer's color buffer
/*! \detailed this buffer _must_ exist when this fct is called, and it
    _must_ have format 'name' */
#define template_writeTile(name, type, cvt)                                    \
  export void LocalFrameBuffer_writeTile_##name(                               \
      void *uniform _fb, uniform Tile &tile)                                   \
  {                                                                            \
    uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;             \
    uniform type *uniform color = (uniform type * uniform) fb->colorBuffer;    \
    uniform float *uniform depth = (uniform float *uniform)fb->depthBuffer;    \
    if (!color)                                                                \
      /* actually, this should never happen ... */                             \
      return;                                                                  \
                                                                               \
    color += (uniform uint64)tile.region.lower.y * fb->super.size.x;           \
    if (depth)                                                                 \
      depth += (uniform uint64)tile.region.lower.y * fb->super.size.x;         \
    VaryingTile *uniform varyTile = (VaryingTile * uniform) & tile;            \
    for (uniform uint32 iiy = tile.region.lower.y; iiy < tile.region.upper.y;  \
         iiy++) {                                                              \
      uniform uint32 chunkID =                                                 \
          (iiy - tile.region.lower.y) * (TILE_SIZE / programCount);            \
      for (uint32 iix = tile.region.lower.x + programIndex;                    \
           iix < tile.region.upper.x;                                          \
           iix += programCount, chunkID++) {                                   \
        unmasked                                                               \
        {                                                                      \
          varying vec4f col = make_vec4f(varyTile->r[chunkID],                 \
              varyTile->g[chunkID],                                            \
              varyTile->b[chunkID],                                            \
              varyTile->a[chunkID]);                                           \
                                                                               \
          const type cvtCol = cvt(col);                                        \
        }                                                                      \
        color[iix] = cvtCol;                                                   \
        if (depth)                                                             \
          depth[iix] = varyTile->z[chunkID];                                   \
      }                                                                        \
      color += fb->super.size.x;                                               \
      if (depth)                                                               \
        depth += fb->super.size.x;                                             \
    }                                                                          \
  }

template_writeTile(RGBA8, uint32, cvt_uint32);
template_writeTile(SRGBA, uint32, linear_to_srgba8);
inline vec4f cvt_nop(const vec4f &v)
{
  return v;
};
template_writeTile(RGBA32F, vec4f, cvt_nop);
#undef template_writeTile

//! \brief accumulate tile into BOTH accum buffer AND tile.
/*! \detailed After this call, the frame buffer will contain 'prev
    accum value + tile value', while the tile will contain '(prev
    accum value + tile value)/numAccums'
   return tile error

Implementation based on
Dammertz et al., "A Hierarchical Automatic Stopping Condition for Monte Carlo
Global Illumination", WSCG 2010 */
export uniform float LocalFrameBuffer_accumulateTile(
    void *uniform _fb, uniform Tile &tile)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  uniform vec4f *uniform accum = fb->accumBuffer;
  if (!accum)
    return inf;

  VaryingTile *uniform varyTile = (VaryingTile * uniform) & tile;
  uniform vec4f *uniform variance = fb->varianceBuffer;

  const uniform float accScale = rcpf(tile.accumID + 1);
  const uniform float accHalfScale = rcpf(tile.accumID / 2 + 1);
  float err = 0.f;

  accum += (uniform uint64)tile.region.lower.y * fb->super.size.x;
  if (variance)
    variance += (uniform uint64)tile.region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile.region.lower.y; iiy < tile.region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile.region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile.region.lower.x + programIndex;
         iix < tile.region.upper.x;
         iix += programCount, chunkID++) {
      /* TODO: rather than gathering, replace this code with
          'load4f's and swizzles */
      varying vec4f acc = make_vec4f(0.f);
      if (tile.accumID > 0)
        acc = accum[iix];
      unmasked
      {
        acc.x += varyTile->r[chunkID];
        acc.y += varyTile->g[chunkID];
        acc.z += varyTile->b[chunkID];
        acc.w += varyTile->a[chunkID];
      }
      accum[iix] = acc;
      acc = acc * accScale;

      // variance buffer accumulates every other frame
      if (variance && (tile.accumID & 1) == 1) {
        varying vec4f vari = make_vec4f(0.f);
        if (tile.accumID > 1)
          vari = variance[iix];
        unmasked
        {
          vari.x += varyTile->r[chunkID];
          vari.y += varyTile->g[chunkID];
          vari.z += varyTile->b[chunkID];
          vari.w += varyTile->a[chunkID];
        }
        variance[iix] = vari;

        // invert alpha (bright alpha is more important)
        const float den2 = reduce_add(make_vec3f(acc)) + (1.f - acc.w);
        if (den2 > 0.0f) {
          const vec4f diff = absf(acc - accHalfScale * vari);
          err += reduce_add(diff) * rsqrtf(den2);
        }
      }

      unmasked
      {
        varyTile->r[chunkID] = acc.x;
        varyTile->g[chunkID] = acc.y;
        varyTile->b[chunkID] = acc.z;
        varyTile->a[chunkID] = acc.w;
      }
    }

    accum += fb->super.size.x;
    if (variance)
      variance += fb->super.size.x;
  }

  const uniform vec2i tileIdx = tile.region.lower / TILE_SIZE;
  const uniform int32 tileId = tileIdx.y * fb->numTiles.x + tileIdx.x;
  fb->tileAccumID[tileId]++;

  // error is also only updated every other frame to avoid alternating error
  // (get a monotone sequence)
  uniform float errf = inf;
  if (variance && (tile.accumID & 1) == 1) {
    uniform vec2i dia = tile.region.upper - tile.region.lower;
    uniform float cntu = (uniform float)dia.x * dia.y;
    // scaling factor to stay backward compatible to default TILE_SIZE=64
    errf = reduce_add(err) * rcp(cntu) * 64.f;
  }
  return errf;
}

// accumulate into a (valid) normal or albedo buffer
export void LocalFrameBuffer_accumulateAuxTile(void *uniform _fb,
    const uniform Tile &tile,
    uniform vec3f *uniform aux,
    const void *uniform _ax,
    const void *uniform _ay,
    const void *uniform _az)
{
  uniform LocalFB *uniform fb = (uniform LocalFB * uniform) _fb;
  const varying float *uniform ax = (const varying float *uniform)_ax;
  const varying float *uniform ay = (const varying float *uniform)_ay;
  const varying float *uniform az = (const varying float *uniform)_az;

  const uniform float accumID = tile.accumID;
  const uniform float accScale = rcpf(tile.accumID + 1);
  aux += (uniform uint64)tile.region.lower.y * fb->super.size.x;

  for (uniform uint32 iiy = tile.region.lower.y; iiy < tile.region.upper.y;
       iiy++) {
    uniform uint32 chunkID =
        (iiy - tile.region.lower.y) * (TILE_SIZE / programCount);

    for (uint32 iix = tile.region.lower.x + programIndex;
         iix < tile.region.upper.x;
         iix += programCount, chunkID++) {
      varying vec3f acc = make_vec3f(0.f);
      if (tile.accumID > 0)
        acc = aux[iix];
      unmasked
      {
        acc.x = (acc.x * accumID + ax[chunkID]) * accScale;
        acc.y = (acc.y * accumID + ay[chunkID]) * accScale;
        acc.z = (acc.z * accumID + az[chunkID]) * accScale;
      }
      aux[iix] = acc;
    }

    aux += fb->super.size.x;
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
    void *uniform normalBuffer,
    void *uniform albedoBuffer,
    void *uniform tileAccumID)
{
  uniform LocalFB *uniform self = uniform new uniform LocalFB;
  FrameBuffer_Constructor(&self->super, cClassPtr);
  FrameBuffer_set(&self->super, size_x, size_y, colorBufferFormat);

  self->colorBuffer = colorBuffer;
  self->depthBuffer = (uniform float *uniform)depthBuffer;
  self->accumBuffer = (uniform vec4f * uniform) accumBuffer;
  self->varianceBuffer = (uniform vec4f * uniform) varianceBuffer;
  self->normalBuffer = (uniform vec3f * uniform) normalBuffer;
  self->albedoBuffer = (uniform vec3f * uniform) albedoBuffer;
  self->numTiles = (self->super.size + (TILE_SIZE - 1)) / TILE_SIZE;
  self->tileAccumID = (uniform int32 * uniform) tileAccumID;

  return self;
}
