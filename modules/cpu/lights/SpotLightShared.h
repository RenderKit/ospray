// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "IntensityDistributionShared.h"
#include "LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// state that changes under transformations
struct SpotLightDynamic
{
  vec3f position;
  vec3f direction;
  vec3f c0;
  vec3f c90;

#ifdef __cplusplus
  SpotLightDynamic()
      : position(0.f),
        direction(0.f, 0.f, 1.f),
        c0(1.f, 0.f, 0.f),
        c90(0.f, 1.f, 0.f)
  {}
#endif // __cplusplus
};

struct SpotLight
{
  Light super;
  vec3f intensity; // RGB radiative intensity of the SpotLight
  vec3f radiance; // emitted RGB radiance
  float cosAngleMax; // Angular limit of the spot in an easier to use form:
                     // cosine of the half angle in radians
  float cosAngleScale; // 1/(cos(border of the penumbra area) - cosAngleMax);
                       // positive
  float radius; // defines the size of the DiskLight represented using
                // the (extended) SpotLight
  float innerRadius; // defines the size of the inner cut out of the DiskLight
                     // to represent a RingLight
  float areaPdf; // pdf of disk/ring with radius/innerRadius
  IntensityDistribution intensityDistribution;
  SpotLightDynamic pre; // un- or pre-transformed state

#ifdef __cplusplus
  SpotLight()
      : intensity(0.f),
        radiance(1.f),
        cosAngleMax(1.f),
        cosAngleScale(1.f),
        radius(0.f),
        innerRadius(0.f),
        areaPdf(inf)
  {
    super.type = LIGHT_TYPE_SPOT;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
