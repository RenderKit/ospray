// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct SciVis
{
  Renderer super;
  bool shadowsEnabled;
  bool visibleLights;
  int aoSamples;
  float aoRadius;
  float volumeSamplingRate;

#ifdef __cplusplus
  SciVis()
      : shadowsEnabled(false),
        visibleLights(false),
        aoSamples(0),
        aoRadius(1e20f),
        volumeSamplingRate(1.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
