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
  Data1D sphere;
  Data1D texcoord;
  Data1D normalData;
  float global_radius;
  OSPSphereType sphereType;

#ifdef __cplusplus
  Spheres() : global_radius(.01f)
  {
    super.type = GEOMETRY_TYPE_SPHERES;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
