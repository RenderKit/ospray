// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/OSPCommon.h"

namespace ospray {

struct FrameBuffer;

/*! A view into a portion of the framebuffer to run the frame operation on
 */
struct FrameBufferView
{
  // TODO Replace w/ arrayview once LocalFB is updated
  // The total dimensions of the global framebuffer
  vec2i fbDims = vec2i(0);
  // The dimensions of this view of the framebuffer
  vec2i viewDims = vec2i(0);
  // The additional halo region pixels included in the view, if requested
  vec2i haloDims = vec2i(0);

  OSPFrameBufferFormat colorBufferFormat = OSP_FB_SRGBA;

  /*! Color buffer of the image, exact pixel format depends
   * on `colorBufferFormat`
   */
  void *colorBuffer = nullptr;
  //! One float per pixel, may be NULL
  float *depthBuffer = nullptr;
  // TODO: Should we pass the accum and variance buffers?
  //! one RGBA per pixel, may be NULL
  // vec4f *accumBuffer;
  //! one RGBA per pixel, may be NULL
  // vec4f *varianceBuffer;
  //! accumulated world-space normal per pixel
  vec3f *normalBuffer = nullptr;
  //! accumulated albedo, one RGBF32 per pixel
  vec3f *albedoBuffer = nullptr;

  FrameBuffer *originalFB = nullptr;

  //! Convenience method to make a view of the entire framebuffer
  FrameBufferView(FrameBuffer *fb,
      OSPFrameBufferFormat colorFormat,
      const vec2i &dims,
      void *colorBuffer,
      float *depthBuffer,
      vec3f *normalBuffer,
      vec3f *albedoBuffer)
      : fbDims(dims),
        viewDims(dims),
        haloDims(0),
        colorBufferFormat(colorFormat),
        colorBuffer(colorBuffer),
        depthBuffer(depthBuffer),
        normalBuffer(normalBuffer),
        albedoBuffer(albedoBuffer),
        originalFB(fb)
  {}

  FrameBufferView() = default;
};

} // namespace ospray
