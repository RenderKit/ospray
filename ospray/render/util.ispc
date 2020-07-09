// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/random.ih"
#include "render/util.ih"

uniform z_order_t z_order;
uniform bool z_order_initialized = false;

void precomputedZOrder_create()
{
  for (uniform uint32 i = 0; i < TILE_SIZE * TILE_SIZE; i++) {
    deinterleaveZOrder(i, &z_order.xs[i], &z_order.ys[i]);
    z_order.xyIdx[i] = z_order.xs[i] | (z_order.ys[i] << 16);
  }

  z_order_initialized = true;
}
