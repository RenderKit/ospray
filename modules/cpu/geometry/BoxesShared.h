// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Boxes
{
  Geometry super;
  Data1D boxes;

#ifdef __cplusplus
  Boxes()
  {
    super.type = GEOMETRY_TYPE_BOXES;
  }
#endif
};
#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
