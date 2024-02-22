// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"
#include "ospray/OSPEnums.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Curves
{
  Geometry super; // inherited geometry fields
  RTCGeometry geom;
  int64 flagMask;
  OSPCurveType curveType;
  OSPCurveBasis curveBasis;

  Data1D index;
  Data1D color;
  Data1D texcoord;

#ifdef __cplusplus
  Curves()
      : geom(nullptr),
        flagMask(-1),
        curveType(OSP_UNKNOWN_CURVE_TYPE),
        curveBasis(OSP_UNKNOWN_CURVE_BASIS)
  {
    super.type = GEOMETRY_TYPE_CURVES;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
