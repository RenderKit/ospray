// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/RendererShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct DistributedRaycastRenderer
{
  Renderer super;
  int aoSamples;
  float aoRadius;
  bool shadowsEnabled;
  float volumeSamplingRate;

#ifdef __cplusplus
  DistributedRaycastRenderer()
      : aoSamples(0),
        aoRadius(1e20f),
        shadowsEnabled(false),
        volumeSamplingRate(1.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
