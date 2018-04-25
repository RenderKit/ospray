// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "render/Renderer.h"

namespace ospray {
  namespace mpi {

    /* The distributed raycast renderer supports rendering distributed
     * geometry and volume data, assuming that the data distribution is suitable
     * for sort-last compositing. Specifically, the data must be organized
     * among nodes such that each nodes region is convex and disjoint from the
     * others. The renderer uses the 'regions' data set on the distributed model
     * from the MPIDistributedDevice to determine the number of tiles to
     * render and expect for compositing.
     *
     * Also see apps/ospRandSciVisTest.cpp and apps/ospRandSphereTest.cpp for
     * example usage.
     */
    struct DistributedRaycastRenderer : public Renderer
    {
      DistributedRaycastRenderer();
      virtual ~DistributedRaycastRenderer() override = default;//TODO!

      void commit() override;

      float renderFrame(FrameBuffer *fb, const uint32 fbChannelFlags) override;

      std::string toString() const override;

      int numAoSamples;
    };

  } // ::ospray::mpi
} // ::ospray
