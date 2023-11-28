// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// No pragma once because this struct is defined twice, both in `ospray` and
// `ispc` namespace

// A view into a portion of the framebuffer to run the frame operation on
// TODO: Change name
struct FrameBufferView
{
  vec2ui fbDims; // the total dimensions of the global framebuffer
  vec2ui viewDims; // the dimensions of this view of the framebuffer

  // Color buffer pointers being input/output to FrameOp
  const vec4f *colorBufferInput;
  vec4f *colorBufferOutput;

  // Other buffers that can used as well in FrameOp
  const float *depthBuffer; // one float per pixel, may be NULL
  const vec3f *normalBuffer; // accumulated world-space normal per pixel
  const vec3f *albedoBuffer; // accumulated albedo, one RGBF32 per pixel

#ifdef __cplusplus
  FrameBufferView() = default;
  FrameBufferView(const vec2ui &dims,
      const vec4f *colorBufferInput,
      const float *depthBuffer = nullptr,
      const vec3f *normalBuffer = nullptr,
      const vec3f *albedoBuffer = nullptr)
      : fbDims(dims),
        viewDims(dims),
        colorBufferInput(colorBufferInput),
        colorBufferOutput(nullptr),
        depthBuffer(depthBuffer),
        normalBuffer(normalBuffer),
        albedoBuffer(albedoBuffer)
  {}
#endif // __cplusplus
};
