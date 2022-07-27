// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBufferView.h"
#include "FrameBuffer.h"

namespace ospray {

FrameBufferView::FrameBufferView(FrameBuffer *fb,
    OSPFrameBufferFormat colorFormat,
    void *colorBuffer,
    float *depthBuffer,
    vec3f *normalBuffer,
    vec3f *albedoBuffer)
    : fbDims(fb->getNumPixels()),
      viewDims(fbDims),
      haloDims(0),
      colorBufferFormat(colorFormat),
      colorBuffer(colorBuffer),
      depthBuffer(depthBuffer),
      normalBuffer(normalBuffer),
      albedoBuffer(albedoBuffer),
      originalFB(fb)
{}

} // namespace ospray
