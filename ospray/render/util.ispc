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

#include "render/util.ih"
#include "math/random.ih"

uniform float precomputedHalton[3][NUM_PRECOMPUTED_HALTON_VALUES];
uniform bool  precomputedHalton_initialized = false;
uniform z_order_t z_order;

static void precomputedHalton_create() {
  if (precomputedHalton_initialized) 
    return;
  // make race conditions less likely by setting flag immediately here at beginning
  precomputedHalton_initialized = true;

  foreach (i=0 ... NUM_PRECOMPUTED_HALTON_VALUES) {
    precomputedHalton[0][i] = radicalInverse2(i+1); // start the sequence with 1
    precomputedHalton[1][i] = CranleyPattersonRotation(radicalInverse(i+1, 3),
        1.f/6.f); // rotate to sample center (0.5) of pixel for i=0
    precomputedHalton[2][i] = radicalInverse(i+1, 5);
  }
}

uniform bool z_order_initialized = false;

void precomputedZOrder_create() {
  for(uniform uint32 i = 0; i < TILE_SIZE*TILE_SIZE; i++) {
    deinterleaveZOrder(i, &z_order.xs[i], &z_order.ys[i]);
    z_order.xyIdx[i] = z_order.xs[i] | (z_order.ys[i] << 16);
  }

  z_order_initialized = true;
}

