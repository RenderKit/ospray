// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistributionShared.h"
#include "LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// state that changes under transformations
struct PointLightDynamic
{
  vec3f position;
  vec3f direction;
  vec3f c0; // orientation for intensityDistribution: direction of the C0-
  vec3f c90; // and the C90-(half)plane
#ifdef __cplusplus
  PointLightDynamic()
      : position(0.f),
        direction(0.f, 0.f, 1.f),
        c0(1.f, 0.f, 0.f),
        c90(0.f, 1.f, 0.f)
  {}
#endif
};

struct PointLight
{
  Light super;
  vec3f intensity; // RGB color and intensity of light
  vec3f radiance; // emitted RGB radiance
  float radius; // defines the size of the SphereLight
  IntensityDistribution intensityDistribution;
  PointLightDynamic pre; // un- or pre-transformed state

#ifdef __cplusplus
  PointLight() : intensity(1.f), radiance(1.f), radius(0.f)
  {
    super.type = LIGHT_TYPE_POINT;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
