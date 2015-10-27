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

/*! \file LoadBalancer.h Implements the abstracion layer for a (tiled) load balancer */

// ospray
#include "ospray/common/OSPCommon.h"
#include "ospray/fb/FrameBuffer.h"
#include "ospray/render/Renderer.h"
#include "ospray/common/TaskSys.h"

// tbb
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>

namespace ospray {


  struct TileRenderer;

  struct TiledLoadBalancer 
  {
    static TiledLoadBalancer *instance;
    virtual std::string toString() const = 0;
    virtual void renderFrame(Renderer *tiledRenderer,
                             FrameBuffer *fb,
                             const uint32 channelFlags) = 0;
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
    rendering on a local machine, without any cross-node
    communication/load balancing at all (even if there are multiple
    application ranks each doing local rendering on their own)  */ 
  struct LocalTiledLoadBalancer : public TiledLoadBalancer
  {
    struct RenderTask
    {
      mutable Ref<FrameBuffer> fb;
      mutable Ref<Renderer>    renderer;
      
      size_t                   numTiles_x;
      size_t                   numTiles_y;
      uint32                   channelFlags;

      void run(size_t jobID) const;
      void finish() const;

      void operator()(const tbb::blocked_range<int>& range) const;
    };

    void renderFrame(Renderer *tiledRenderer,
                     FrameBuffer *fb,
                     const uint32 channelFlags);
    std::string toString() const { return "ospray::LocalTiledLoadBalancer"; };
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
    rendering on a local machine, without any cross-node
    communication/load balancing at all (even if there are multiple
    application ranks each doing local rendering on their own)  */ 
  struct InterleavedTiledLoadBalancer : public TiledLoadBalancer
  {
    size_t deviceID;
    size_t numDevices;

    InterleavedTiledLoadBalancer(size_t deviceID, size_t numDevices)
      : deviceID(deviceID), numDevices(numDevices)
    {
      if (ospray::debugMode || ospray::logLevel) {
        std::cout << "=======================================================" << std::endl;
        std::cout << "INTERLEAVED LOAD BALANCER" << std::endl;
        std::cout << "=======================================================" << std::endl;
      }
    }

    virtual std::string toString() const { return "ospray::InterleavedTiledLoadBalancer"; };

    /*! \brief a task for rendering a frame using the global tiled load balancer 
      
      if derived from FrameBuffer::RenderFrameEvent to allow us for
      attaching that as a sync primitive to the frame buffer
    */
    struct RenderTask : public Task {
      Ref<FrameBuffer> fb;
      Ref<Renderer>    renderer;
      
      size_t           numTiles_x;
      size_t           numTiles_y;
      size_t           numTiles_mine;
      size_t           deviceID;
      size_t           numDevices;
      uint32           channelFlags;

      void run(size_t jobID);
      void finish();
    };
    
    void renderFrame(Renderer *tiledRenderer,
                     FrameBuffer *fb,
                     const uint32 channelFlags);
  };

} // ::ospray
