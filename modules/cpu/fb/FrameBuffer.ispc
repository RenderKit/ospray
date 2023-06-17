// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameBuffer.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f FrameBuffer_runPixelOps(FrameBuffer *uniform self,
    const vec4f &color,
    const float depth,
    const vec3f &normal,
    const vec3f &albedo)
{
  vec4f outColor = color;
#ifndef OSPRAY_TARGET_SYCL
  for (uniform int i = 0; i < self->numPixelOps; ++i) {
    // TODO: PixelOps need porting to run via dispatch
    outColor = self->pixelOps[i]->processPixel(
        self->pixelOps[i], outColor, depth, normal, albedo);
  }
#else
  // Silence unused parameter warning
  (void)self;
  (void)depth;
  (void)normal;
  (void)albedo;
#endif
  return outColor;
}
OSPRAY_END_ISPC_NAMESPACE
