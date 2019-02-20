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

#include <memory>
#include <fstream>
// ospray
#include "render/Renderer.h"
#include "camera/PerspectiveCamera.h"
#include "../../common/DistributedModel.h"

namespace ospray {
  namespace mpi {

    // A region is defined by its bounds and an ID, which allows us to group
    // ranks with the same region and switch to do image-parallel rendering
    struct DistributedRegion
    {
      box3f bounds;
      int id;

      DistributedRegion();
      DistributedRegion(box3f bounds, int id);

      bool operator==(const ospray::mpi::DistributedRegion &b) const;
      bool operator<(const ospray::mpi::DistributedRegion &b) const;
    };

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
      virtual ~DistributedRaycastRenderer() override;

      void commit() override;

      float renderFrame(FrameBuffer *fb, const uint32 fbChannelFlags) override;

      // TODO WILL: This is only for benchmarking the compositing using
      // the same rendering code path. Remove it once we're done! Or push
      // it behind some ifdefs!
      float renderNonDistrib(FrameBuffer *fb, const uint32 fbChannelFlags);

      std::string toString() const override;

    private:
      // Send my bounding boxes to other nodes, receive theirs for a
      // "full picture" of what geometries live on what nodes
      void exchangeModelBounds();

      int numAoSamples;
      bool oneSidedLighting;
      bool shadowsEnabled;
      PerspectiveCamera *camera;
      std::unique_ptr<std::ofstream> statsLog;

      vec3f ambientLight;
      Data *lightData;
      std::vector<void*> lightIEs;

      std::vector<DistributedModel*> regions, ghostRegions;
      std::vector<void*> regionIEs, ghostRegionIEs;
      // The global list of unique regions across all nodes, (including this one),
      // sorted by region id.
      std::vector<DistributedRegion> allRegions;
      // The ranks which own each region
      std::unordered_map<int, std::set<size_t>> regionOwners;
    };

  } // ::ospray::mpi
} // ::ospray

std::ostream& operator<<(std::ostream &os, const ospray::mpi::DistributedRegion &d);
bool operator==(const ospray::mpi::DistributedRegion &a,
                const ospray::mpi::DistributedRegion &b);
bool operator<(const ospray::mpi::DistributedRegion &a,
               const ospray::mpi::DistributedRegion &b);

