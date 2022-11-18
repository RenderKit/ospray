// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct AORenderer
{
  Renderer super;
  int aoSamples;
  float aoRadius;
  float aoIntensity;
  float volumeSamplingRate;

#ifdef __cplusplus
  AORenderer()
      : aoSamples(1), aoRadius(1e20f), aoIntensity(1.f), volumeSamplingRate(1.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
