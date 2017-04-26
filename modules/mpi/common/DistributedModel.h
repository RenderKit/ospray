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

// ospray stuff
#include "geometry/Geometry.h"
#include "volume/Volume.h"

// stl
#include <vector>

// embree
#include "embree2/rtcore.h"

namespace ospray {
  namespace mpi {

    struct DistributedModel : public ManagedObject
    {
      DistributedModel();
      virtual ~DistributedModel() = default;

      //! \brief common function to help printf-debugging
      virtual std::string toString() const override;
      virtual void commit() override;

      // Data members //

      //! \brief vector of all geometries used in this model
      std::vector<Ref<Geometry>> geometry;

      //! \brief the embree scene handle for this geometry
      RTCScene embreeSceneHandle {nullptr};
      box3f bounds;
    };

  } // ::ospray::mpi
} // ::ospray
