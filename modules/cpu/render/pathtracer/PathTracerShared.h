// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct PathTracer
{
  Renderer super;

  uint32 rouletteDepth; // path depth from which on RR is used
  uint32 maxScatteringEvents;
  float maxRadiance;
  // coefficients of plane equation defining geometry to catch shadows for
  // compositing; disabled if normal is zero-length
  vec4f shadowCatcherPlane;
  bool shadowCatcher; // preprocessed
  bool backgroundRefraction;
  int32 numLightSamples; // number of light samples used for NEE

#ifdef __cplusplus
  PathTracer()
      : rouletteDepth(5),
        maxScatteringEvents(20),
        maxRadiance(inf),
        shadowCatcherPlane(0.f),
        shadowCatcher(false),
        backgroundRefraction(false),
        numLightSamples(1)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
