// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct IntensityDistribution
{
  float *lid; // luminance intensity distribution
  vec2i size;
  vec2f scale;

#ifdef __cplusplus
  IntensityDistribution() : lid{nullptr}, size(0), scale(1.f) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
