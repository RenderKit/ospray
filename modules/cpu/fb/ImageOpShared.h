// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
#include "rkcommon/math/vec.h"
namespace ispc {
#else
#include "rkcommon/math/vec.ih"
#endif

#if defined(__cplusplus) && !defined(OSPRAY_TARGET_DPCPP)
typedef void *LivePixelOp_processPixel;
#else
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
