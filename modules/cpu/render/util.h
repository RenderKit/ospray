// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

/*! \file render/util.h Defines some utility functions shared by different
 * shading codes */

#include "common/OSPCommon.h"

namespace ospray {

// TODO: Could use pdep/pext
inline uint32_t partitionZOrder(uint32_t n)
{
  n &= 0x0000FFFF;
  n = (n | (n << 8)) & 0x00FF00FF;
  n = (n | (n << 4)) & 0x0F0F0F0F;
  n = (n | (n << 2)) & 0x33333333;
  n = (n | (n << 1)) & 0x55555555;
  return n;
}

inline uint32_t unpartitionZOrder(uint32_t n)
{
  n &= 0x55555555;
  n = (n ^ (n >> 1)) & 0x33333333;
  n = (n ^ (n >> 2)) & 0x0F0F0F0F;
  n = (n ^ (n >> 4)) & 0x00FF00FF;
  n = (n ^ (n >> 8)) & 0x0000FFFF;
  return n;
}

inline uint32_t interleaveZOrder(uint32_t x, uint32_t y)
{
  return partitionZOrder(x) | (partitionZOrder(y) << 1);
}

inline void deinterleaveZOrder(uint32_t z, uint32_t &x, uint32_t &y)
{
  x = y = 0;
  x = unpartitionZOrder(z);
  y = unpartitionZOrder(z >> 1);
}

} // namespace ospray
