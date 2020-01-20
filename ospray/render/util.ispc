// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "render/util.ih"

uniform float precomputedHalton[3][NUM_PRECOMPUTED_HALTON_VALUES];
uniform bool precomputedHalton_initialized = false;
uniform z_order_t z_order;

static void precomputedHalton_create()
{
  if (precomputedHalton_initialized)
    return;
  // make race conditions less likely by setting flag immediately here at
  // beginning
  precomputedHalton_initialized = true;

  foreach (i = 0 ... NUM_PRECOMPUTED_HALTON_VALUES) {
    precomputedHalton[0][i] =
        radicalInverse2(i + 1); // start the sequence with 1
    precomputedHalton[1][i] = CranleyPattersonRotation(radicalInverse(i + 1, 3),
        1.f / 6.f); // rotate to sample center (0.5) of pixel for i=0
    precomputedHalton[2][i] = radicalInverse(i + 1, 5);
  }
}

uniform bool z_order_initialized = false;

void precomputedZOrder_create()
{
  for (uniform uint32 i = 0; i < TILE_SIZE * TILE_SIZE; i++) {
    deinterleaveZOrder(i, &z_order.xs[i], &z_order.ys[i]);
    z_order.xyIdx[i] = z_order.xs[i] | (z_order.ys[i] << 16);
  }

  z_order_initialized = true;
}
