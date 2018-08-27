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

#include <ostream>
#include <unordered_map>
#include <set>

// ospray stuff
#include "geometry/Geometry.h"
#include "volume/Volume.h"
#include "common/Model.h"

// stl
#include <vector>

// embree
#include "embree3/rtcore.h"

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

    struct DistributedModel : public Model
    {
      DistributedModel();
      virtual ~DistributedModel() override = default;

      virtual std::string toString() const override;

      // commit synchronizes the distributed models between processes
      // so that ranks know how many tiles to expect for sort-last
      // compositing.
      virtual void commit() override;

      // The local regions on this node
      std::vector<DistributedRegion> myRegions;
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

