// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// for very small cones treat as singular light, because float precision is not
// good enough
#define COS_ANGLE_MAX 0.99999988f

struct DirectionalLight
{
  Light super; // inherited light fields

  linear3f frame; // coordinate frame, with vz == direction *towards* the
                  // light source
  vec3f irradiance; // RGB irradiance contribution of the light
  float cosAngle; // Angular limit of the cone light in an easier to use form:
                  // cosine of the half angle in radians
  float pdf; // Probability to sample a direction to the light

#ifdef __cplusplus
  DirectionalLight() : frame(one), irradiance(0.f), cosAngle(1.f), pdf(inf)
  {
    super.type = LIGHT_TYPE_DIRECTIONAL;
  }
  void set(bool isVisible,
      const Instance *instance,
      const vec3f &direction,
      const vec3f &irradiance,
      float cosAngle);
};
} // namespace ispc
#else
};
#endif // __cplusplus
