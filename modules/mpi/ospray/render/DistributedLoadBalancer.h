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

#include "../common/DistributedWorld.h"
#include "../fb/DistributedFrameBuffer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "render/LoadBalancer.h"

namespace ospray {
  namespace mpi {
    namespace staticLoadBalancer {

      /* The distributed load balancer manages both data and image
       * parallel distributed rendering, based on the renderer and
       * world configuration. Distributed renderers will use distributed
       * rendering, while a non-distributed renderer with a world with
       * one global region will give image-parallel rendering
       */
      struct Distributed : public TiledLoadBalancer
      {
        float renderFrame(FrameBuffer *fb,
                          Renderer *renderer,
                          Camera *camera,
                          World *world) override;

        float renderFrameReplicated(DistributedFrameBuffer *dfb,
                                    Renderer *renderer,
                                    Camera *camera,
                                    DistributedWorld *world);

        std::string toString() const override;
      };

    }  // namespace staticLoadBalancer
  }    // namespace mpi
}  // namespace ospray
