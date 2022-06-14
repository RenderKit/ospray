// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Curves
{
  Geometry super; // inherited geometry fields
  RTCGeometry geom;
  int64 flagMask;

#ifdef __cplusplus
  Curves() : geom(nullptr), flagMask(-1) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
