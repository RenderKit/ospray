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

#include <ostream>
#include <unordered_map>
#include <set>

// ospray stuff
#include "geometry/Geometry.h"
#include "volume/Volume.h"
#include "common/Model.h"

// ospray common
#include "ospcommon/box.h"

// stl
#include <vector>

// embree
#include "embree3/rtcore.h"

namespace ospray {
  namespace mpi {

    struct DistributedModel : public Model
    {
      DistributedModel();
      virtual ~DistributedModel() override = default;

      virtual std::string toString() const override;

      // commit synchronizes the distributed models between processes
      // so that ranks know how many tiles to expect for sort-last
      // compositing.
      virtual void commit() override;

      int id;
    };

  } // ::ospray::mpi
} // ::ospray
