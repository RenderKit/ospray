// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "fb/FrameBuffer.ih"
#include "math/vec.ih"

struct DistributedFrameBuffer
{
  FrameBuffer super;
};

// accumulates the colors in a tile, writes the final normalized colors into a
// separate tile, and computes the error
export uniform float DFB_accumulateTile(const VaryingTile *uniform tile,
                                        VaryingTile *uniform final,
                                        VaryingTile *uniform accum,
                                        VaryingTile *uniform variance,
                                        uniform bool hasAccumBuffer,
                                        uniform bool hasVarianceBuffer)
{
  const uniform int accumID = tile->accumID;
  const uniform vec2i size  = box_size(tile->region);
  const uniform int maxi    = size.y * (TILE_SIZE / programCount);
  uniform float errf        = inf;
  /* variance buffer accumulates every other frame */
  const uniform bool accumulateVariance =
      hasVarianceBuffer && (accumID & 1) == 1;
  if (!hasAccumBuffer || accumID < 1) {
    for (uniform int i = 0; i < maxi; i++) {
      vec4f col   = make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]);
      accum->r[i] = col.x;
      accum->g[i] = col.y;
      accum->b[i] = col.z;
      accum->a[i] = col.w;
      accum->z[i] = tile->z[i];
      final->r[i] = col.x;
      final->g[i] = col.y;
      final->b[i] = col.z;
      final->a[i] = col.w;
      final->z[i] = tile->z[i];
    }
  } else {
    const uniform float rcpAccumID   = rcpf(accumID + 1);
    const uniform float accHalfScale = rcpf(accumID / 2 + 1);
    float err                        = 0.f;
    for (uniform int i = 0; i < maxi; i++) {
      vec4f col = make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]);
      vec4f acc =
          col + make_vec4f(accum->r[i], accum->g[i], accum->b[i], accum->a[i]);

      accum->r[i] = acc.x;
      accum->g[i] = acc.y;
      accum->b[i] = acc.z;
      accum->a[i] = acc.w;

      acc = acc * rcpAccumID;

      if (accumulateVariance) {
        varying vec4f vari = make_vec4f(0.f);
        if (accumID > 1)
          vari = make_vec4f(
              variance->r[i], variance->g[i], variance->b[i], variance->a[i]);
        vari           = vari + col;
        variance->r[i] = vari.x;
        variance->g[i] = vari.y;
        variance->b[i] = vari.z;
        variance->a[i] = vari.w;

        /* invert alpha (bright alpha is more important */
        const float den2 = reduce_add(make_vec3f(acc)) + (1.f - acc.w);
        if (den2 > 0.0f) {
          const vec4f diff = absf(acc - accHalfScale * vari);
          if ((i * programCount % TILE_SIZE) + programIndex < size.x)
            err += reduce_add(diff) * rsqrtf(den2);
        }
      }

      final->r[i] = acc.x;
      final->g[i] = acc.y;
      final->b[i] = acc.z;
      final->a[i] = acc.w;
    }
    /* error is also only updated every other frame to avoid alternating
     * error (get a monotone sequence) */
    if (accumulateVariance)
      errf = reduce_add(err) * rsqrtf(size.x * size.y);
  }
  return errf;
}

// variant that only accumulates (without computing error)
// assumption: there is always ACCUM and VARIANCE buffer (otherwise variance
// will always be inf and WriteMultipleTiles mode won't be used)
export void DFB_accumulateTileSimple(const VaryingTile *uniform tile,
                                     VaryingTile *uniform accum,
                                     VaryingTile *uniform variance)
{
  // variance buffer accumulates every other frame
  const uniform bool accumulateVariance = (tile->accumID & 1) == 1;
  const uniform vec2i size              = box_size(tile->region);
  const uniform int maxi                = size.y * (TILE_SIZE / programCount);

  for (uniform int i = 0; i < maxi; i++) {
    vec4f col = make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]);

    accum->r[i] += col.x;
    accum->g[i] += col.y;
    accum->b[i] += col.z;
    accum->a[i] += col.w;

    if (accumulateVariance) {
      variance->r[i] += col.x;
      variance->g[i] += col.y;
      variance->b[i] += col.z;
      variance->a[i] += col.w;
    }
  }
}

// accumulate normal and albedo
export void DFB_accumulateAuxTile(const VaryingTile *uniform tile,
                                  Tile *uniform final,
                                  VaryingTile *uniform accum)
{
  const uniform vec2i size = box_size(tile->region);
  const uniform int maxi   = size.y * (TILE_SIZE / programCount);
  if (tile->accumID < 1)
    for (uniform int i = 0; i < maxi; i++) {
      const float fnx = tile->nx[i];
      const float fny = tile->ny[i];
      const float fnz = tile->nz[i];
      accum->nx[i]    = fnx;
      accum->ny[i]    = fny;
      accum->nz[i]    = fnz;
      // TODO: Is it not safe to assume the nx/ny/nz are packed in memory?
      // It seems like this soa to aos conversion should not be needed
      soa_to_aos3(fnx, fny, fnz, final->nx + 3 * programCount * i);
      const float far = tile->ar[i];
      const float fag = tile->ag[i];
      const float fab = tile->ab[i];
      accum->ar[i]    = far;
      accum->ag[i]    = fag;
      accum->ab[i]    = fab;
      soa_to_aos3(far, fag, fab, final->ar + 3 * programCount * i);
    }
  else {
    const uniform float rcpAccumID = rcpf(tile->accumID + 1);
    for (uniform int i = 0; i < maxi; i++) {
      accum->nx[i] += tile->nx[i];
      accum->ny[i] += tile->ny[i];
      accum->nz[i] += tile->nz[i];
      const float fnx = accum->nx[i] * rcpAccumID;
      const float fny = accum->ny[i] * rcpAccumID;
      const float fnz = accum->nz[i] * rcpAccumID;
      soa_to_aos3(fnx, fny, fnz, final->nx + 3 * programCount * i);
      accum->ar[i] += tile->ar[i];
      accum->ag[i] += tile->ag[i];
      accum->ab[i] += tile->ab[i];
      const float far = accum->ar[i] * rcpAccumID;
      const float fag = accum->ag[i] * rcpAccumID;
      const float fab = accum->ab[i] * rcpAccumID;
      soa_to_aos3(far, fag, fab, final->ar + 3 * programCount * i);
    }
  }
}

// computes the error for a tile
export uniform float DFB_computeErrorForTile(const uniform vec2i &size,
                                             const VaryingTile *uniform accum,
                                             const VaryingTile *uniform
                                                 variance,
                                             const uniform float accumID)
{
  const uniform int maxi           = size.y * (TILE_SIZE / programCount);
  const uniform float rcpAccumID   = rcpf(accumID + 1);
  const uniform float accHalfScale = rcpf((accumID + 1) / 2);
  float err                        = 0.f;
  for (uniform int i = 0; i < maxi; i++) {
    vec4f acc = make_vec4f(accum->r[i], accum->g[i], accum->b[i], accum->a[i]);
    acc       = acc * rcpAccumID;

    /* invert alpha (bright alpha is more important */
    const float den2 = reduce_add(make_vec3f(acc)) + (1.f - acc.w);
    if (den2 > 0.0f) {
      const vec4f vari = make_vec4f(
          variance->r[i], variance->g[i], variance->b[i], variance->a[i]);
      const vec4f diff = absf(acc - accHalfScale * vari);
      if ((i * programCount % TILE_SIZE) + programIndex < size.x)
        err += reduce_add(diff) * rsqrtf(den2);
    }
  }
  return reduce_add(err) * rsqrtf(size.x * size.y);
}

// writes the final normalized colors in a tile into a color buffer
#define template_writeTile(name, dst_fmt, cvt)                        \
  export void DFB_writeTile_##name(const VaryingTile *uniform tile,   \
                                   void *uniform _color)              \
  {                                                                   \
    const uniform vec2i size = box_size(tile->region);                \
    const uniform int maxi   = size.y * (TILE_SIZE / programCount);   \
    dst_fmt *uniform color   = (dst_fmt * uniform) _color;            \
    for (uniform int i = 0; i < maxi; i++) {                          \
      const vec4f c =                                                 \
          make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]); \
      color[i] = cvt(c);                                              \
    }                                                                 \
  }

template_writeTile(RGBA8, varying uint32, cvt_uint32);
template_writeTile(SRGBA, varying uint32, linear_to_srgba8);
inline vec4f soa_to_aos4f(const vec4f s)
{
  vec4f a;
  soa_to_aos4(s.x, s.y, s.z, s.w, (uniform float *uniform) & a);
  return a;
}
template_writeTile(RGBA32F, varying vec4f, soa_to_aos4f);
#undef template_writeTile

export void DFB_zComposite(const VaryingTile *uniform delta,
                           VaryingTile *uniform current)
{
  for (uniform int i = 0; i < TILE_SIZE * TILE_SIZE / programCount; i++) {
    const bool closer = delta->z[i] < current->z[i];
    current->r[i]     = closer ? delta->r[i] : current->r[i];
    current->g[i]     = closer ? delta->g[i] : current->g[i];
    current->b[i]     = closer ? delta->b[i] : current->b[i];
    current->a[i]     = closer ? delta->a[i] : current->a[i];
    current->z[i]     = closer ? delta->z[i] : current->z[i];
  }
}

export void *uniform DFB_create(void *uniform cClassPtr)
{
  DistributedFrameBuffer *uniform self =
      uniform new uniform DistributedFrameBuffer;
  FrameBuffer_Constructor(&self->super, cClassPtr);

  return self;
}

export void DFB_set(void *uniform _self,
                    const uniform uint32 size_x,
                    const uniform uint32 size_y,
                    uniform int32 colorBufferFormat)
{
  DistributedFrameBuffer *uniform self = (DistributedFrameBuffer *)_self;
  FrameBuffer_set(&self->super, size_x, size_y, colorBufferFormat);
}

inline void swapFragments(VaryingTile *uniform t0,
                          VaryingTile *uniform t1,
                          uniform int fragID)
{
  float r       = t0->r[fragID];
  t0->r[fragID] = t1->r[fragID];
  t1->r[fragID] = r;
  float g       = t0->g[fragID];
  t0->g[fragID] = t1->g[fragID];
  t1->g[fragID] = g;
  float b       = t0->b[fragID];
  t0->b[fragID] = t1->b[fragID];
  t1->b[fragID] = b;
  float a       = t0->a[fragID];
  t0->a[fragID] = t1->a[fragID];
  t1->a[fragID] = a;
  float z       = t0->z[fragID];
  t0->z[fragID] = t1->z[fragID];
  t1->z[fragID] = z;
}

#define PIXELS_PER_BLEND_JOB 64

inline void sortAndBlendFragments(VaryingTile *uniform *uniform tileArray,
                                  uniform int32 numTiles,
                                  uniform int begin_frag,
                                  uniform int end_frag)
{
  for (uniform int frag = begin_frag; frag < end_frag; ++frag) {
#if 1
    for (uniform int i = 1; i < numTiles; ++i) {
      for (uniform int j = i; j > 0; --j) {
        VaryingTile *uniform ta = tileArray[j];
        VaryingTile *uniform tb = tileArray[j - 1];
        if (ta->z[frag] < tb->z[frag])
          break;
        swapFragments(ta, tb, frag);
      }
    }
#endif

    vec3f acc_col   = make_vec3f(0, 0, 0);
    float acc_alpha = 0.f;

    for (uniform int i = 0; i < numTiles; ++i) {
      VaryingTile *uniform tile = tileArray[i];
      vec3f frag_col = make_vec3f(tile->r[frag], tile->g[frag], tile->b[frag]);
      float frag_alpha = tile->a[frag];

      // Note: back-to-front blending
      acc_col   = frag_col + (1.f - frag_alpha) * acc_col;
      acc_alpha = frag_alpha + (1.f - frag_alpha) * acc_alpha;
    }

    VaryingTile *uniform tile = tileArray[0];
    tile->r[frag]             = acc_col.x;
    tile->g[frag]             = acc_col.y;
    tile->b[frag]             = acc_col.z;
    tile->a[frag]             = acc_alpha;
    // Since we blend back-to-front, the nearest depth sample is
    // on the last tile after sorting.
    tile->z[frag] = tileArray[numTiles - 1]->z[frag];
  }
}

export void DFB_sortAndBlendFragments(VaryingTile *uniform *uniform tileArray,
                                      uniform int32 numTiles)
{
  sortAndBlendFragments(
      tileArray, numTiles, 0, TILE_SIZE * TILE_SIZE / programCount);
}
