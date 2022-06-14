// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
typedef void *LivePixelOp_processPixel;
#else
#include "rkcommon/math/vec.ih"
struct LivePixelOp;

typedef vec4f (*LivePixelOp_processPixel)(const LivePixelOp *uniform self,
    const vec4f &color,
    const float depth,
    const vec3f &normal,
    const vec3f &albedo);
#endif

struct LivePixelOp
{
  LivePixelOp_processPixel processPixel;

#ifdef __cplusplus
  LivePixelOp() : processPixel(nullptr) {}
#endif
};

#ifdef __cplusplus
}
#endif
