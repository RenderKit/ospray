// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "common/Managed.h"

namespace ospray {

  struct FrameBuffer;

  /*! A view into a portion of the frambuffer to run the frame operation on
   */
  struct OSPRAY_SDK_INTERFACE FrameBufferView
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

    //! Convenience method to make a view of the entire framebuffer
    FrameBufferView(FrameBuffer *fb,
                    OSPFrameBufferFormat colorFormat,
                    void *colorBuffer,
                    float *depthBuffer,
                    vec3f *normalBuffer,
                    vec3f *albedoBuffer);
    FrameBufferView() = default;
  };

}  // namespace ospray
