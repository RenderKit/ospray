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
#include "ospray/fb/PixelOp.h"

namespace ospray {

  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    /*! app-mappable format of the color buffer. make sure that this
        matches the definition on the ISPC side */
    typedef OSPFrameBufferFormat ColorBufferFormat;

    const vec2i size;
    FrameBuffer(const vec2i &size,
                ColorBufferFormat colorBufferFormat,
                bool hasDepthBuffer,
                bool hasAccumBuffer,
                bool hasVarianceBuffer = false);

    virtual const void *mapDepthBuffer() = 0;
    virtual const void *mapColorBuffer() = 0;

    virtual void unmap(const void *mappedMem) = 0;
    virtual void setTile(Tile &tile) = 0;

    /*! \brief clear (the specified channels of) this frame buffer */
    virtual void clear(const uint32 fbChannelFlags) = 0;

    //! get number of pixels per tile, in x and y direction
    vec2i getTileSize()  const { return vec2i(TILE_SIZE); }

    //! return number of tiles in x and y direction
    vec2i getNumTiles()  const { return numTiles; }

    int getTotalTiles()  const { return numTiles.x * numTiles.y; }

    //! get number of pixels in x and y diretion
    vec2i getNumPixels() const { return size; }

    vec2i numTiles;
    vec2i maxValidPixelID;

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should overrride this! */
    virtual std::string toString() const
    { return "ospray::FrameBuffer"; }

    /*! indicates whether the app requested this frame buffer to have
        an (application-mappable) depth buffer */
    bool hasDepthBuffer;
    /*! indicates whether the app requested this frame buffer to have
        an accumulation buffer */
    bool hasAccumBuffer;
    bool hasVarianceBuffer;

    /*! buffer format of the color buffer */
    ColorBufferFormat colorBufferFormat;

    /*! how often has been accumulated into that tile
        Note that it is up to the application to properly
        reset the accumulationIDs (using ospClearAccum(fb)) if anything
        changes that requires clearing the accumulation buffer. */
    virtual int32 accumID(const vec2i &tile) = 0;
    virtual float tileError(const vec2i &tile) = 0;

    //! returns error of frame
    virtual float endFrame(const float errorThreshold) = 0;

    Ref<PixelOp::Instance> pixelOp;
  };

  /*! helper function for debugging. write out given pixels in PPM
      format. Pixels are supposed to come in as RGBA, 4x8 bit */
  void writePPM(const std::string &fileName, const vec2i &size, uint32 *pixels);

  //! helper function to write a (float) image as (flipped) PFM file
  void writePFM(const std::string &fileName, const vec2i &size,
                const int channel, const float *pixels);

} // ::ospray
