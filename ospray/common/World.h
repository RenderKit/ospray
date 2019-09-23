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

// ospray stuff
#include "./Data.h"
#include "./Managed.h"
#include "Instance.h"
// stl
#include <vector>
// embree
#include "embree3/rtcore.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE World : public ManagedObject
  {
    World();
    virtual ~World() override;

    std::string toString() const override;
    void commit() override;

    // Data members //

    Ref<const DataT<Instance *>> instances;
    std::vector<void*> instanceIEs;
    int numGeometries{0};
    int numVolumes{0};

    //! \brief the embree scene handle for this geometry
    RTCScene embreeSceneHandleGeometries{nullptr};
    RTCScene embreeSceneHandleVolumes{nullptr};
  };

  OSPTYPEFOR_SPECIALIZATION(World *, OSP_WORLD);

}  // namespace ospray
