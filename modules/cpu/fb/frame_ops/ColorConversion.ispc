// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DeviceRT.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "ColorConversionShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

inline void ColorConversion_kernel(
    const vec2ui itemIndex, const FrameBufferView *uniform fbvSelf)
{
  const LiveColorConversion *uniform self =
      (const LiveColorConversion *uniform)fbvSelf;
  const uint32 i = itemIndex.y * fbvSelf->viewDims.x + itemIndex.x;
  vec4f v = fbvSelf->colorBufferInput[i];
  switch (self->targetColorFormat) {
  case OSP_FB_RGBA8: {
    uint32 *uniform typedCB = (uint32 * uniform) self->convBuffer;
    typedCB[i] = cvt_uint32(v);
  } break;
  case OSP_FB_SRGBA: {
    uint32 *uniform typedCB = (uint32 * uniform) self->convBuffer;
    typedCB[i] = linear_to_srgba8(v);
  } break;
  case OSP_FB_RGBA32F: {
    // Should never get here
  } break;
  default:
    // Should never get here
    break;
  }
}

DEFINE_FRAMEOP_KERNEL_LAUNCHER(ColorConversion_kernel);

OSPRAY_END_ISPC_NAMESPACE
