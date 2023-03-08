// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Distribution2D
{
  vec2i size;
  // 1/size
  vec2f rcpSize;
  // size.x*size.y elements
  float *cdf_x;
  // size.y elements
  float *cdf_y;

#ifdef __cplusplus
  Distribution2D() : size(0), rcpSize(0.f), cdf_x(nullptr), cdf_y(nullptr) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
