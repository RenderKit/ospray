// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Distribution1D.ih"

#ifdef OSPRAY_TARGET_SYCL
#include <limits>
#endif

// Pre-condition: size > 0

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL export uniform float Distribution1D_create(
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
#ifdef OSPRAY_TARGET_SYCL
  const float nextAfter1 = 1.f + std::numeric_limits<float>::epsilon();
#else
  const uniform float nextAfter1 = 0x1.000002p+0f;
#endif

  // normalize
#ifdef OSPRAY_TARGET_SYCL
  for (int i = 0; i < size; ++i) {
#else
  foreach (i = 0 ... size) {
#endif
    const float c = cdf[i];
    // handle cornercases (sum=0 -> rcpSum=inf; correct termination of sampling
    // with s=1.0f)
    const float q = c >= sum ? nextAfter1 : c * rcpSum;
    cdf[i] = q;
  }

  return sum;
}
OSPRAY_END_ISPC_NAMESPACE
