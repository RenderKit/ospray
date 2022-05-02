// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "fb/FrameBuffer.ih"

vec4f FrameBuffer_runPixelOps(FrameBuffer *uniform self,
    const vec4f &color,
    const float depth,
    const vec3f &normal,
    const vec3f &albedo)
{
  vec4f outColor = color;
  for (uniform int i = 0; i < self->numPixelOps; ++i) {
    outColor = self->pixelOps[i]->processPixel(
        self->pixelOps[i], outColor, depth, normal, albedo);
  }
  return outColor;
}
