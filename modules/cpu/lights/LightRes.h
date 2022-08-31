// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/rkcommonSYCLWrappers.h"

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#else
#include "rkcommon/math/vec.ih"
#endif // __cplusplus

struct Light_SampleRes
{
  vec3f weight; // radiance that arrives at the given point divided by pdf
  vec3f dir; // direction towards the light source, normalized
  float dist; // largest valid t_far value for a shadow ray, including epsilon
              // to avoid self-intersection
  float pdf; // probability density that this sample was taken
};

struct Light_EvalRes
{
  vec3f radiance; // radiance that arrives at the given point (not weighted by
                  // pdf)
  float pdf; // probability density that the direction would have been sampled
};
#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
