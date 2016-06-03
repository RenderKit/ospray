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

#include "ospray/fb/FrameBuffer.ih"
#include "ospray/math/vec.ih"

struct DistributedFrameBuffer {
  FrameBuffer super;
};

export void DFB_alphaBlendBackground(VaryingTile    *uniform firstTile,
                                     const uniform vec4f &bgColor)
{
  /*! note: alpha=1 means "fully opaque" */
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    vec4f col_over = make_vec4f(firstTile->r[i],
                                firstTile->g[i],
                                firstTile->b[i],
                                firstTile->a[i]);
    vec4f col_new = col_over + bgColor * (1.f-col_over.w);
    firstTile->r[i] = col_new.x;
    firstTile->g[i] = col_new.y;
    firstTile->b[i] = col_new.z;
    firstTile->a[i] = col_new.w;
  }
}

export void DFB_alphaBlendTiles(VaryingTile    *uniform prev,
                                VaryingTile    *uniform over)
{
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    vec4f col_prev = make_vec4f(prev->r[i], prev->g[i], prev->b[i], prev->a[i]);
    vec4f col_over = make_vec4f(over->r[i], over->g[i], over->b[i], over->a[i]);
    vec4f col_new = col_over + col_prev * (1.f-col_over.w);
    prev->r[i] = col_new.x;
    prev->g[i] = col_new.y;
    prev->b[i] = col_new.z;
    prev->a[i] = col_new.w;
  }
}

#define template_accumulate(name, dst_fmt, cvt)                              \
export void DFB_accumulate_##name(void           *uniform _self,             \
                                  VaryingTile    *uniform tile,              \
                                  VaryingTile    *uniform final,             \
                                  VaryingTile    *uniform accum,             \
                                  void           *uniform _color,            \
                                  uniform int     accumID,                   \
                                  uniform bool    hasAccumBuffer)            \
{                                                                            \
  DistributedFrameBuffer *uniform self = (DistributedFrameBuffer*)_self;     \
  dst_fmt *uniform color = (dst_fmt*uniform)_color;                          \
  if (!hasAccumBuffer || accumID < 1) {                                      \
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {           \
      vec4f col = make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]);\
      accum->r[i] = col.x;                                                   \
      accum->g[i] = col.y;                                                   \
      accum->b[i] = col.z;                                                   \
      accum->a[i] = col.w;                                                   \
      final->r[i] = col.x;                                                   \
      final->g[i] = col.y;                                                   \
      final->b[i] = col.z;                                                   \
      final->a[i] = col.w;                                                   \
                                                                             \
      color[i] = cvt(col);                                                   \
    }                                                                        \
  } else {                                                                   \
    const uniform float rcpAccumID = 1.f/(accumID+1);                        \
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {           \
      vec4f col = make_vec4f(tile->r[i], tile->g[i], tile->b[i], tile->a[i]) \
        + make_vec4f(accum->r[i], accum->g[i], accum->b[i], accum->a[i]);    \
                                                                             \
      accum->r[i] = col.x;                                                   \
      accum->g[i] = col.y;                                                   \
      accum->b[i] = col.z;                                                   \
      accum->a[i] = col.w;                                                   \
                                                                             \
      col = col * rcpAccumID;                                                \
                                                                             \
      final->r[i] = col.x;                                                   \
      final->g[i] = col.y;                                                   \
      final->b[i] = col.z;                                                   \
      final->a[i] = col.w;                                                   \
                                                                             \
      color[i] = cvt(col);                                                   \
    }                                                                        \
  }                                                                          \
}

inline vec4f soa_to_aos4f(const vec4f s)
{
  vec4f a;
  soa_to_aos4(s.x, s.y, s.z, s.w, (uniform float*uniform)&a);
  return a;
}

template_accumulate(RGBA8, varying uint32, cvt_uint32);
template_accumulate(SRGBA, varying uint32, linear_to_srgba8);
template_accumulate(RGBA32F, varying vec4f, soa_to_aos4f);
#undef template_accumulate


export void DFB_zComposite(VaryingTile    *uniform delta,
                           VaryingTile    *uniform current)
{
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const bool closer = delta->z[i] < current->z[i];
    current->r[i] = closer ? delta->r[i] : current->r[i];
    current->g[i] = closer ? delta->g[i] : current->g[i];
    current->b[i] = closer ? delta->b[i] : current->b[i];
    current->a[i] = closer ? delta->a[i] : current->a[i];
    current->z[i] = closer ? delta->z[i] : current->z[i];
  }
}


export void *uniform DFB_create(void *uniform cClassPtr)
{
  DistributedFrameBuffer *uniform self =
      uniform new uniform DistributedFrameBuffer;
  FrameBuffer_Constructor(&self->super,cClassPtr);

  return self;
}

export void DFB_set(void *uniform _self,
                    const uniform uint32 size_x,
                    const uniform uint32 size_y,
                    uniform int32 colorBufferFormat)
{
  DistributedFrameBuffer *uniform self = (DistributedFrameBuffer*)_self;
  FrameBuffer_set(&self->super,size_x,size_y,colorBufferFormat);
}

inline void swapFragments(VaryingTile *uniform t0,
                          VaryingTile *uniform t1,
                          uniform int fragID)
{
  float r = t0->r[fragID]; t0->r[fragID] = t1->r[fragID]; t1->r[fragID] = r;
  float g = t0->g[fragID]; t0->g[fragID] = t1->g[fragID]; t1->g[fragID] = g;
  float b = t0->b[fragID]; t0->b[fragID] = t1->b[fragID]; t1->b[fragID] = b;
  float a = t0->a[fragID]; t0->a[fragID] = t1->a[fragID]; t1->a[fragID] = a;
  float z = t0->z[fragID]; t0->z[fragID] = t1->z[fragID]; t1->z[fragID] = z;
}



#define PIXELS_PER_BLEND_JOB 64

inline void sortAndBlendFragments(VaryingTile *uniform *uniform tileArray,
                                  uniform int32 numTiles,
                                  uniform int begin_frag, uniform int end_frag)
{
  for (uniform int frag=begin_frag;frag<end_frag;frag++) {
    for (uniform int i=1;i<numTiles;i++)
      for (uniform int j=i;j>0;--j) {
        VaryingTile *uniform ta = tileArray[j];
        VaryingTile *uniform tb = tileArray[j-1];
        if (ta->z[frag] < tb->z[frag])
          break;
        swapFragments(ta,tb,frag);
      }

    vec3f acc_col = make_vec3f(0,0,0);
    float acc_alpha = 0.f;

    for (uniform int i=0;i<numTiles;i++) {
      VaryingTile *uniform tile = tileArray[i];
      vec3f frag_col = make_vec3f(tile->r[frag],tile->g[frag],tile->b[frag]);
      float frag_alpha = tile->a[frag];

      acc_col = //frag_alpha * 
        frag_col + (1.f-frag_alpha) * acc_col;
      acc_alpha = //frag_alpha * 
        frag_alpha + (1.f-frag_alpha) * acc_alpha;
    }
    
    VaryingTile *uniform tile = tileArray[0];
    tile->r[frag] = acc_col.x;
    tile->g[frag] = acc_col.y;
    tile->b[frag] = acc_col.z;
    tile->a[frag] = acc_alpha;
  }
}

#if 0
task void DFB_sortAndBlendFragments_job(VaryingTile *uniform *uniform tileArray,
                                        uniform int32 numTiles)
{
  uniform int begin = taskIndex * (PIXELS_PER_BLEND_JOB/programCount);
  uniform int end = begin + (PIXELS_PER_BLEND_JOB/programCount);
  sortAndBlendFragments(tileArray,numTiles,begin,end);
}
#endif

export void DFB_sortAndBlendFragments(VaryingTile *uniform *uniform tileArray,
                                      uniform int32 numTiles)
{
// NOTE(jda) - I do not know if this should be removed...?
#if 1
  sortAndBlendFragments(tileArray,numTiles,0,TILE_SIZE*TILE_SIZE/programCount);
#else
  uniform int numJobs = (TILE_SIZE*TILE_SIZE) / PIXELS_PER_BLEND_JOB;
  launch[numJobs] DFB_sortAndBlendFragments_job(tileArray,numTiles);
#endif
}


