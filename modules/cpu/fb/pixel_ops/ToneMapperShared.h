// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "fb/PixelOpShared.h"

#ifdef __cplusplus
namespace ispc {
#endif

// Based on the generic filmic tone mapping operator from
// [Lottes, 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"]
struct LiveToneMapper
{
  LivePixelOp super;
  // linear exposure adjustment
  float exposure;
  // coefficients
  float a, b, c, d;
  // ACES color transform flag
  bool acesColor;
};
#ifdef __cplusplus
}
#endif
