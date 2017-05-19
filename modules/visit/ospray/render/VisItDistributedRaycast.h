// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
  namespace visit {

    /* The distributed raycast renderer supports rendering distributed
     * geometry and volume data, assuming that the data distribution is suitable
     * for sort-last compositing. Specifically, the data must be organized
     * among nodes such that each nodes region is convex and disjoint from the
     * others. In the case of overlapping geometry (ghost zones, etc.) you
     * can specify the 'clipBox.lower' and 'clipBox.upper' parameters to clip
     * the geometry to only find hits within this node's region.
     * Only one volume per node is currently supported.
     *
     * Also see apps/ospRandSciVisTest.cpp and apps/ospRandSphereTest.cpp for
     * example usage.
     */
    struct VisItDistributedRaycastRenderer : public Renderer
    {
      VisItDistributedRaycastRenderer();
      virtual ~VisItDistributedRaycastRenderer() = default; //TODO!

      void commit() override;

      float renderFrame(FrameBuffer *fb, const uint32 fbChannelFlags) override;

      std::string toString() const override;
    };

  } // ::ospray::visit
} // ::ospray
