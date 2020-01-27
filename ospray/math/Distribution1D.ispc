// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Distribution1D.ih"

uniform float Distribution1D_create(
    const uniform int size, uniform float *uniform cdf)
{
  // accumulate the function
  // using varying float and exclusive_scan_add(float v) is much slower
  uniform float sum = cdf[0];
  for (uniform int i = 1; i < size; i++)
    cdf[i] = sum += cdf[i];

  // compute reciprocal sum
  const uniform float rcpSum = 1.0f / sum;
  // next representable number in float greater than 1.0f
  const uniform float nextAfter1 = 0x1.000002p+0f;

  // normalize
  foreach (i = 0 ... size) {
    const float c = cdf[i];
    // handle cornercases (sum=0 -> rcpSum=inf; correct termination of sampling
    // with s=1.0f)
    const float q = c >= sum ? nextAfter1 : c * rcpSum;
    cdf[i] = q;
  }

  return sum;
}
