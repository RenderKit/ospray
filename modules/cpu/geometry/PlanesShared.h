// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Planes
{
  Geometry super;
  Data1D coeffs;
  Data1D bounds;
};

#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
