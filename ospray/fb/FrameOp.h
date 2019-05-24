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

// ospray
#include "common/Managed.h"

namespace ospray {

  struct FrameBuffer;

  /*! A view into a portion of the frambuffer to run the frame operation on
   */
  struct OSPRAY_SDK_INTERFACE FrameBufferView {
    // The total dimensions of the global framebuffer
    vec2i fbDims;
    // The dimensions of this view of the framebuffer
    vec2i viewDims;
    // The additional halo region pixels included in the view, if requested
    vec2i haloDims;

    OSPFrameBufferFormat colorBufferFormat;
    /*! Color buffer of the image, exact pixel format depends
     * on `colorBufferFormat`
     */
    void  *colorBuffer;
    //! One float per pixel, may be NULL
    float *depthBuffer;
    // TODO: Should we pass the accum and variance buffers?
    //! one RGBA per pixel, may be NULL
    //vec4f *accumBuffer;
    //! one RGBA per pixel, may be NULL
    //vec4f *varianceBuffer;
    //! accumulated world-space normal per pixel
    vec3f *normalBuffer;
    //! accumulated albedo, one RGBF32 per pixel
    vec3f *albedoBuffer;

    //! Convenience method to make a view of the entire framebuffer 
    FrameBufferView(FrameBuffer *fb, OSPFrameBufferFormat colorFormat,
                    void *colorBuffer, float *depthBuffer,
                    vec3f *normalBuffer, vec3f *albedoBuffer);
  };

  /*! TODO docs */
  struct OSPRAY_SDK_INTERFACE FrameOp : public ManagedObject
  {
      virtual ~FrameOp() override = default;

      /*! TODO docs */
      virtual void endFrame(FrameBufferView &fb) {}

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;

      static FrameOp *createInstance(const char *type);
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      frameop. By having this symbol in the shared lib ospray can
      later on always get a handle to this fct and create an instance
      of this frameop.
  */
#define OSP_REGISTER_FRAME_OP(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(FrameOp, frame_op, InternalClass, external_name)


  struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp {
      void endFrame(FrameBufferView &fb) override;

      std::string toString() const override;
  };

}

