// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This header is shared with ISPC
// No pragma once because this struct is defined twice, both in `ospray` and
// `ispc` namespace

// A view into a portion of the framebuffer to run the frame operation on
struct FrameBufferView
{
  // The total dimensions of the global framebuffer
  vec2ui fbDims;
  // The dimensions of this view of the framebuffer
  vec2ui viewDims;

  // Accumulated color
  vec4f *colorBuffer;
  // One float per pixel, may be NULL
  float *depthBuffer;
  // Accumulated world-space normal per pixel
  vec3f *normalBuffer;
  // Accumulated albedo, one RGBF32 per pixel
  vec3f *albedoBuffer;

  // Convenience method to make a view of the entire framebuffer
#ifdef __cplusplus
  FrameBufferView(const vec2ui &dims,
      vec4f *colorBuffer,
      float *depthBuffer,
      vec3f *normalBuffer,
      vec3f *albedoBuffer)
      : fbDims(dims),
        viewDims(dims),
        colorBuffer(colorBuffer),
        depthBuffer(depthBuffer),
        normalBuffer(normalBuffer),
        albedoBuffer(albedoBuffer)
  {}
#endif // __cplusplus
};
