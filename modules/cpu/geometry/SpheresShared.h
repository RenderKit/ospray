// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Spheres
{
  Geometry super;
  Data1D vertex;
  Data1D radius;
  Data1D texcoord;
  Data1D tex1d;   //ensight's colorby attribute
  Data1D atex1d;  //ensight's alphaby attribute
  float global_radius;

#ifdef __cplusplus
  Spheres() : global_radius(.01f) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
