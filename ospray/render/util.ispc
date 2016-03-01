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

#include "ospray/render/util.ih"
#include "ospray/math/random.ih"

uniform float precomputedHalton[3][NUM_PRECOMPUTED_HALTON_VALUES];
uniform bool  precomputedHalton_initialized = false;
uniform z_order_t z_order;

static void precomputedHalton_create() {
  if (precomputedHalton_initialized) 
    return;
  // make race conditions less likely by setting flag immediatley here at beginning
  precomputedHalton_initialized = true;

  foreach (i=0 ... NUM_PRECOMPUTED_HALTON_VALUES) {
    precomputedHalton[0][i] = radicalInverse2(i);
    precomputedHalton[1][i] = radicalInverse(i,3);
    precomputedHalton[2][i] = radicalInverse(i,5);
  }
};

uniform bool z_order_initialized = false;

inline uniform uint32 partition(uniform uint32 n) {
  n &= 0x0000FFFF;
  n = (n | (n << 8)) & 0x00FF00FF;
  n = (n | (n << 4)) & 0x0F0F0F0F;
  n = (n | (n << 2)) & 0x33333333;
  n = (n | (n << 1)) & 0x55555555;
  return n;
}

inline uniform uint32 unpartition(uniform uint32 n) {
  n &= 0x55555555;
  n = (n ^ (n >> 1)) & 0x33333333;
  n = (n ^ (n >> 2)) & 0x0F0F0F0F;
  n = (n ^ (n >> 4)) & 0x00FF00FF;
  n = (n ^ (n >> 8)) & 0x0000FFFF;
  return n;
}

inline uniform uint32 interleave(uniform uint32 x, uniform uint32 y) {
  return partition(x) | (partition(y) << 1);
}

inline void deinterleave(uniform uint32 z, uniform uint32 *uniform x, uniform uint32 *uniform y) {
  assert(x); assert(y);
  *x = *y = 0;
  *x = unpartition(z);
  *y = unpartition(z >> 1);
}

void precomputedZOrder_create() {
  for(uniform uint32 i = 0; i < TILE_SIZE*TILE_SIZE; i++) {
    deinterleave(i, &z_order.xs[i], &z_order.ys[i]);
    z_order.xyIdx[i] = z_order.xs[i] | (z_order.ys[i] << 16);
  }

  z_order_initialized = true;
}

