// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistributionShared.h"
#include "LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// state that changes under transformations
struct QuadLightDynamic
{
  vec3f position;
  vec3f edge1;
  vec3f edge2;
  vec3f nnormal; // negated normal, the direction that the QuadLight is not
                 // emitting; normalized
  float ppdf; // probability to sample point on light = 1/area
  vec3f c0; // orientation for intensityDistribution: direction of the C0-
  vec3f c90; // and the C90-(half)plane

#ifdef __cplusplus
  QuadLightDynamic()
      : position(0.f),
        edge1(1.f, 0.f, 0.f),
        edge2(0.f, 1.f, 0.f),
        nnormal(0.f, 0.f, 1.f),
        ppdf(1.f),
        c0(0.f, 1.f, 0.f),
        c90(1.f, 0.f, 0.f)
  {}
#endif
};

struct QuadLight
{
  Light super;
  vec3f radiance; // emitted RGB radiance
  IntensityDistribution intensityDistribution;
  QuadLightDynamic pre; // un- or pre-transformed state

#ifdef __cplusplus
  QuadLight() : radiance(0.f)
  {
    super.type = LIGHT_TYPE_QUAD;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
