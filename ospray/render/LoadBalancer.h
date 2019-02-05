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

/*! \file LoadBalancer.h Implements the abstracion layer for a (tiled) load balancer */

// ospray
#include "common/OSPCommon.h"
#include "fb/FrameBuffer.h"
#include "render/Renderer.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE TiledLoadBalancer
  {
    virtual ~TiledLoadBalancer() {}
    static std::unique_ptr<TiledLoadBalancer> instance;
    virtual std::string toString() const = 0;
    virtual float renderFrame(Renderer *tiledRenderer,
                              FrameBuffer *fb,
                              const uint32 channelFlags) = 0;

    static size_t numJobs(const int spp, int accumID)
    {
      const int blocks = (accumID > 0 || spp > 0) ? 1 :
        std::min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);
      return divRoundUp((TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB, blocks);
    }
  };

  //! tiled load balancer for local rendering on the given machine
  /*! a tiled load balancer that orchestrates (multi-threaded)
    rendering on a local machine, without any cross-node
    communication/load balancing at all (even if there are multiple
    application ranks each doing local rendering on their own)  */
  struct OSPRAY_SDK_INTERFACE LocalTiledLoadBalancer : public TiledLoadBalancer
  {
    float renderFrame(Renderer *renderer,
                      FrameBuffer *fb,
                      const uint32 channelFlags) override;

    std::string toString() const override;
  };

} // ::ospray
