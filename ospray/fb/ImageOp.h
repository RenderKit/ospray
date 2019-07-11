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
#include "fb/Tile.h"

namespace ospray {

  struct FrameBuffer;

  /*! A view into a portion of the frambuffer to run the frame operation on
   */
  struct OSPRAY_SDK_INTERFACE FrameBufferView {
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
    void  *colorBuffer = nullptr;
    //! One float per pixel, may be NULL
    float *depthBuffer = nullptr;
    // TODO: Should we pass the accum and variance buffers?
    //! one RGBA per pixel, may be NULL
    //vec4f *accumBuffer;
    //! one RGBA per pixel, may be NULL
    //vec4f *varianceBuffer;
    //! accumulated world-space normal per pixel
    vec3f *normalBuffer = nullptr;
    //! accumulated albedo, one RGBF32 per pixel
    vec3f *albedoBuffer = nullptr;

    //! Convenience method to make a view of the entire framebuffer 
    FrameBufferView(FrameBuffer *fb, OSPFrameBufferFormat colorFormat,
                    void *colorBuffer, float *depthBuffer,
                    vec3f *normalBuffer, vec3f *albedoBuffer);
    FrameBufferView() = default;
  };

  /*! An instance of an image op which is actually attached to a framebuffer */
  struct LiveImageOp
  {
    FrameBufferView fbView;

    LiveImageOp(FrameBufferView &fbView);

    virtual ~LiveImageOp() {}

    /*! gets called once at the beginning of the frame */
    virtual void beginFrame() {}

    /*! gets called once at the end of the frame */
    virtual void endFrame() {}
  };

  /*! \brief base abstraction for a "Image Op" to be performed for
      every image that gets written into a frame buffer.

      A ImageOp is basically a "hook" that allows to inject arbitrary
      code, such as postprocessing, filtering, blending, tone mapping,
      sending tiles to a display wall, etc.
  */
  struct OSPRAY_SDK_INTERFACE ImageOp : public ManagedObject
  {
      virtual ~ImageOp() override = default;

      //! \brief common function to help printf-debugging
      /*! Every derived class should override this! */
      virtual std::string toString() const override;

      /*! Attach an image op to an existing framebuffer. Use this
       * to pass the params from the API to the instance of the image op
       * which will actually be run on the framebuffer view or tiles of the
       * framebuffer passed
       */
      virtual std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) = 0;

      static ImageOp *createInstance(const char *type);
  };

  struct OSPRAY_SDK_INTERFACE TileOp : public ImageOp
  {
    virtual ~TileOp() {}
  };

  struct OSPRAY_SDK_INTERFACE FrameOp : public ImageOp
  {
    virtual ~FrameOp() {}
    virtual vec2i haloSize() { return vec2i(0); }
  };

  struct OSPRAY_SDK_INTERFACE LiveTileOp : public LiveImageOp
  {
    LiveTileOp(FrameBufferView &fbView);
    virtual ~LiveTileOp() {}

    /*! called right after the tile got accumulated; i.e., the
      tile's RGBA values already contain the accu-buffer blended
      values (assuming an accubuffer exists), and this function
      defines how these pixels are being processed before written
      into the color buffer */
    virtual void process(Tile &) = 0;
  };

  struct OSPRAY_SDK_INTERFACE LiveFrameOp : public LiveImageOp
  {
    LiveFrameOp(FrameBufferView &fbView);
    virtual ~LiveFrameOp() {}

    virtual void process() = 0;
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      imageop. By having this symbol in the shared lib ospray can
      later on always get a handle to this fct and create an instance
      of this imageop.
  */
#define OSP_REGISTER_IMAGE_OP(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(ImageOp, image_op, InternalClass, external_name)
}

