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

/*! the ispc equivalent of the c-side
  DistributedFrameBuffer::DFBTileData as it is sent over the
  network */
struct DFBTileData {
  varying float  accum_r[TILE_SIZE*TILE_SIZE/programCount];
  varying float  accum_g[TILE_SIZE*TILE_SIZE/programCount];
  varying float  accum_b[TILE_SIZE*TILE_SIZE/programCount];
  varying float  accum_a[TILE_SIZE*TILE_SIZE/programCount];
  varying uint32 color[TILE_SIZE*TILE_SIZE/programCount];
};

export void DFB_accumTile(VaryingTile *uniform tile,
                          DFBTileData *uniform td,
                          uniform bool hasAccumBuffer,
                          uniform int32 accumID)
{
  const float rcpAccumID = 1.f/(accumID+1);
  for (uniform int i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    vec4f col = make_vec4f(tile->r[i],
                           tile->g[i],
                           tile->b[i],
                           tile->a[i]);
    if (hasAccumBuffer) {
      if (accumID > 0) {
        col = col + make_vec4f(td->accum_r[i],
                               td->accum_g[i],
                               td->accum_b[i],
                               td->accum_a[i]);
      }
      td->accum_r[i] = col.x;
      td->accum_g[i] = col.y;
      td->accum_b[i] = col.z;
      td->accum_a[i] = col.w;
      col = col * rcpAccumID;
    }
    td->color[i] = cvt_uint32(col);
  }
}

export void *uniform DistributedFrameBuffer_create(void *uniform cClassPtr)
{
  DistributedFrameBuffer *uniform self = uniform new uniform DistributedFrameBuffer;
  FrameBuffer_Constructor(&self->super,cClassPtr);

  return self;
}

export void DistributedFrameBuffer_set(void *uniform _self,
                                       const uniform uint32 size_x,
                                       const uniform uint32 size_y,
                                       uniform int32 colorBufferFormat)
{
  DistributedFrameBuffer *uniform self = (DistributedFrameBuffer*)_self;
  FrameBuffer_set(&self->super,size_x,size_y,colorBufferFormat);
}



