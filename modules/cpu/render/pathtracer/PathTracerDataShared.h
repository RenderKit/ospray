// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

struct Light;

struct PathtracerData
{
  // array containing the scene light sources
  // the lights are sorted: first geometric, then virtual lights
  Light **lights;
  uint32 numLights; // total number of light sources (geometric + virtual)
  uint32 numGeoLights; // number of geometric light sources
  float *lightsCDF; // CDF used by NEE for randomly picking lights

#ifdef __cplusplus
  PathtracerData()
      : lights(nullptr), numLights(0), numGeoLights(0), lightsCDF(nullptr)
  {}
  void create(
      Light **lights, uint32 numLights, uint32 numGeoLights, float *lightsCDF);
  void destroy();
};
} // namespace ispc
#else
};
#endif // __cplusplus
