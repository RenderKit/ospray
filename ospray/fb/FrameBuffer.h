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

// ospray
#include "Tile.h"

// ospray
#include "../common/OSPCommon.h"
#include "../common/Managed.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {

  using embree::TaskScheduler;

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
    virtual const void *mapDepthBuffer() = 0;
    virtual const void *mapColorBuffer() = 0;

    virtual void unmap(const void *mappedMem) = 0;

    /*! make sure the current frame is finished */
    void waitForRenderTaskToBeReady();

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

    virtual void clear(const uint32 fbChannelFlags) = 0;
  };

  
  /*! local frame buffer - frame buffer that exists on local machine */
  struct LocalFrameBuffer : public FrameBuffer {
    void      *colorBuffer; /*!< format depends on
                               FrameBuffer::colorBufferFormat, may be
                               NULL */
    float     *depthBuffer; /*!< one float per pixel, may be NULL */
    vec4f     *accumBuffer; /*!< one RGBA per pixel, may be NULL */

    LocalFrameBuffer(const vec2i &size,
                     ColorBufferFormat colorBufferFormat,
                     bool hasDepthBuffer,
                     bool hasAccumBuffer, 
                     void *colorBufferToUse=NULL);
    virtual ~LocalFrameBuffer();
    
    virtual const void *mapColorBuffer();
    virtual const void *mapDepthBuffer();
    virtual void unmap(const void *mappedMem);
    virtual void clear(const uint32 fbChannelFlags);
  };

} // ::ospray
