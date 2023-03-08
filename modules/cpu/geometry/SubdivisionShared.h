// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Subdivision
{
  Geometry super;
  RTCGeometry geom;
  int64 flagMask; // which attributes are missing and cannot be interpolated

#ifdef __cplusplus
  Subdivision() : geom(nullptr), flagMask(-1)
  {
    super.type = GEOMETRY_TYPE_SUBDIVISION;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
