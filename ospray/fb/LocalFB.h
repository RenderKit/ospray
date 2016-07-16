// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "fb/FrameBuffer.h"

namespace ospray {

  /*! local frame buffer - frame buffer that exists on local machine */
  struct LocalFrameBuffer : public FrameBuffer {
    void      *colorBuffer; /*!< format depends on
                               FrameBuffer::colorBufferFormat, may be
                               NULL */
    float     *depthBuffer; /*!< one float per pixel, may be NULL */
    vec4f     *accumBuffer; /*!< one RGBA per pixel, may be NULL */
    vec4f     *varianceBuffer; /*!< one RGBA per pixel, may be NULL, accumulates every other sample, for variance estimation / stopping */
    int32     *tileAccumID; //< holds accumID per tile, for adaptive accumulation
    float     *tileErrorBuffer; /*!< holds error per tile, for variance estimation / stopping */
    std::vector<box2i> errorRegion; // image regions (in #tiles) which do not yet estimate the error on tile base

    LocalFrameBuffer(const vec2i &size,
                     ColorBufferFormat colorBufferFormat,
                     bool hasDepthBuffer,
                     bool hasAccumBuffer,
                     bool hasVarianceBuffer,
                     void *colorBufferToUse=NULL);
    virtual ~LocalFrameBuffer();

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should overrride this! */
    std::string toString() const override;

    void setTile(Tile &tile) override;
    int32 accumID(const vec2i &tile) override;
    float tileError(const vec2i &tile) override;
    float endFrame(const float errorThreshold) override;

    const void *mapColorBuffer() override;
    const void *mapDepthBuffer() override;
    void unmap(const void *mappedMem) override;
    void clear(const uint32 fbChannelFlags) override;
  };

} // ::ospray
