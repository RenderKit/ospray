// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

struct VaryingRGBA_I8 {
  varying uint32 color[TILE_SIZE*TILE_SIZE/programCount];
};
// /*! the ispc equivalent of the c-side
//   DistributedFrameBuffer::DFBTileData as it is sent over the
//   network */
// struct DFBTileData {
//   VaryingTile accum;
//   // varying float  accum_r[TILE_SIZE*TILE_SIZE/programCount];
//   // varying float  accum_g[TILE_SIZE*TILE_SIZE/programCount];
//   // varying float  accum_b[TILE_SIZE*TILE_SIZE/programCount];
//   // varying float  accum_a[TILE_SIZE*TILE_SIZE/programCount];
//   varying uint32 color[TILE_SIZE*TILE_SIZE/programCount];
// };

export void DFB_accumulate(VaryingTile    *uniform tile,
                           VaryingTile    *uniform final,
                           VaryingTile    *uniform accum,
                           VaryingRGBA_I8 *uniform color,
                           uniform bool hasAccumBuffer,
                           uniform int32 accumID)
{
  if (!hasAccumBuffer || accumID < 1) {
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      vec4f col = make_vec4f(tile->r[i],
                             tile->g[i],
                             tile->b[i],
                             tile->a[i]);
      accum->r[i] = col.x;
      accum->g[i] = col.y;
      accum->b[i] = col.z;
      accum->a[i] = col.w;
      final->r[i] = col.x;
      final->g[i] = col.y;
      final->b[i] = col.z;
      final->a[i] = col.w;
      color->color[i] = cvt_uint32(col);
    }
  } else {
    const float rcpAccumID = 1.f/(accumID+1);
    for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
      vec4f col
        = make_vec4f(tile->r[i],
                     tile->g[i],
                     tile->b[i],
                     tile->a[i])
        + make_vec4f(accum->r[i],
                     accum->g[i],
                     accum->b[i],
                     accum->a[i]);
      
      accum->r[i] = col.x;
      accum->g[i] = col.y;
      accum->b[i] = col.z;
      accum->a[i] = col.w;
      
      col = col * rcpAccumID;
      
      final->r[i] = col.x;
      final->g[i] = col.y;
      final->b[i] = col.z;
      final->a[i] = col.w;
      color->color[i] = cvt_uint32(col);
    }
  }
}


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
  DistributedFrameBuffer *uniform self = uniform new uniform DistributedFrameBuffer;
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



