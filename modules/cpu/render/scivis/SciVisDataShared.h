// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Light;

struct SciVisData
{
  // array containing the scene light sources
  // the lights are sorted: first lights that are not sampled (visible only),
  // then lights that are both sampled and visible
  Light **lights;
  uint32 numLights; // total number of light sources
  uint32 numLightsVisibleOnly; // number of lights that are not sampled
                               // (visible only)
  vec3f aoColorPi;

#ifdef __cplusplus
  SciVisData()
      : lights(nullptr), numLights(0), numLightsVisibleOnly(0), aoColorPi(0.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
