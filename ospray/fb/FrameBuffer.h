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

#include <atomic>
// ospray
#include "common/Managed.h"
#include "fb/PixelOp.h"
#include "ospray/ospray.h"

namespace ospray {

  /*! abstract frame buffer class */
  struct OSPRAY_SDK_INTERFACE FrameBuffer : public ManagedObject
  {
    /*! app-mappable format of the color buffer. make sure that this
        matches the definition on the ISPC side */
    using ColorBufferFormat = OSPFrameBufferFormat;

    FrameBuffer(const vec2i &size,
                ColorBufferFormat colorBufferFormat,
                const uint32 channels);
    virtual ~FrameBuffer() override = default;

    virtual const void *mapBuffer(OSPFrameBufferChannel channel) = 0;

    virtual void unmap(const void *mappedMem) = 0;
    virtual void setTile(Tile &tile)          = 0;

    /*! \brief clear (the specified channels of) this frame buffer */
    virtual void clear() = 0;

    //! get number of pixels per tile, in x and y direction
    vec2i getTileSize() const;

    //! return number of tiles in x and y direction
    vec2i getNumTiles() const;

    int getTotalTiles() const;

    //! get number of pixels in x and y diretion
    vec2i getNumPixels() const;

    float getVariance() const;

    /*! how often has been accumulated into that tile
        Note that it is up to the application to properly
        reset the accumulationIDs (using ospClearAccum(fb)) if anything
        changes that requires clearing the accumulation buffer. */
    virtual int32 accumID(const vec2i &tile)   = 0;
    virtual float tileError(const vec2i &tile) = 0;

    virtual void beginFrame();

    //! returns error of frame
    virtual void endFrame(const float errorThreshold) = 0;

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should override this! */
    virtual std::string toString() const override;

    void setCompletedEvent(OSPSyncEvent event);
    OSPSyncEvent getLatestCompleteEvent() const;
    void waitForEvent(OSPSyncEvent event) const;

    void reportProgress(float newValue);
    float getCurrentProgress();

    void cancelFrame();
    bool frameCancelled() const;

   protected:
    const vec2i size;
    vec2i numTiles;
    vec2i maxValidPixelID;

    /*! buffer format of the color buffer */
    ColorBufferFormat colorBufferFormat;

    /*! indicates whether the app requested this frame buffer to have
        an (application-mappable) depth buffer */
    bool hasDepthBuffer;
    /*! indicates whether the app requested this frame buffer to have
        an accumulation buffer */
    bool hasAccumBuffer;
    bool hasVarianceBuffer;
    bool hasNormalBuffer;
    bool hasAlbedoBuffer;

    //! This marks the global number of frames that have been rendered since
    //! the last ospFramebufferClear() has been called.
    int32 frameID;

    float frameVariance{0.f};

    std::atomic<float> frameProgress{1.f};
    std::atomic<bool> cancelRender{false};

    std::atomic<OSPSyncEvent> stagesCompleted{OSP_FRAME_FINISHED};

   public:  // TODO: make this private!
    Ref<PixelOp::Instance> pixelOp;
  };
}  // namespace ospray
