// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lights/LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct GeometricModel;

// The GeometryLight is a proxy object fulfilling the Light.ih API.
// It is generated internally for each emitting geometry modelance to facilitate
// explicit importance sampling (currently wrt. the surface area).
struct GeometryLight
{
  Light super; // inherited light fields

  const GeometricModel *model; // underlying geometry
  int32 numPrimitives; // number of emissive primitives
  int32 *primIDs; // IDs of emissive primitives to sample
  float *distribution; // pdf over prims proportional to (world-space) area
  float pdf; // probability density to sample point on surface := 1/area

#ifdef __cplusplus
  GeometryLight()
      : model(nullptr),
        numPrimitives(0),
        primIDs(nullptr),
        distribution(nullptr),
        pdf(inf)
  {
    super.type = LIGHT_TYPE_GEOMETRY;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
