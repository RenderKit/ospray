// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// std
#include <algorithm>

// ospray
#include "ospray/fb/PixelOp.h"

namespace ospray {

  /*! helper function to convert float-color into rgba-uint format */
  inline uint32 cvt_uint32(const float f)
  {
    return (int32)(255.9f * std::max(std::min(f,1.f),0.f));
  }

  /*! helper function to convert float-color into rgba-uint format */
  inline uint32 cvt_uint32(const vec4f &v)
  {
    return 
      (cvt_uint32(v.x) << 0)  |
      (cvt_uint32(v.y) << 8)  |
      (cvt_uint32(v.z) << 16) |
      (cvt_uint32(v.w) << 24);
  }

  /*! helper function to convert float-color into rgba-uint format */
  inline uint32 cvt_uint32(const vec3f &v)
  {
    return 
      (cvt_uint32(v.x) << 0)  |
      (cvt_uint32(v.y) << 8)  |
      (cvt_uint32(v.z) << 16);
  }



  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    /*! app-mappable format of the color buffer. make sure that this
        matches the definition on the ISPC side */
    typedef OSPFrameBufferFormat ColorBufferFormat;
    
    const vec2i size;
    FrameBuffer(const vec2i &size,
                ColorBufferFormat colorBufferFormat,
                bool hasDepthBuffer,
                bool hasAccumBuffer);

    virtual void commit();

    virtual const void *mapDepthBuffer() = 0;
    virtual const void *mapColorBuffer() = 0;

    virtual void unmap(const void *mappedMem) = 0;
    virtual void setTile(Tile &tile) = 0;

    /*! \brief clear (the specified channels of) this frame buffer */
    virtual void clear(const uint32 fbChannelFlags) = 0;

    /*! indicates whether the app requested this frame buffer to have
        an accumulation buffer */
    bool hasAccumBuffer;
    /*! indicates whether the app requested this frame buffer to have
        an (application-mappable) depth buffer */
    bool hasDepthBuffer;

    /*! buffer format of the color buffer */
    ColorBufferFormat colorBufferFormat;

    /*! tracks how many times we have already accumulated into this
        frame buffer. A value of '<0' means that accumulation is
        disabled (in which case the renderer may not access the
        accumulation buffer); in all other cases this value indicates
        how many frames have already been accumulated in this frame
        buffer. Note that it is up to the application to properly
        reset the accumulationID (using ospClearAccum(fb)) if anything
        changes that requires clearing the accumulation buffer. */
    int32 accumID;

    Ref<PixelOp::Instance> pixelOp;
  };

  /*! helper function for debugging. write out given pixels in PPM
      format. Pixels are supposed to come in as RGBA, 4x8 bit */
  void writePPM(const std::string &fileName, const vec2i &size, uint32 *pixels);
  
} // ::ospray
