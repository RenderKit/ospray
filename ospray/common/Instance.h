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
#include "./Group.h"
// embree
#include "embree3/rtcore.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE Instance : public ManagedObject
  {
    Instance(Group *group);
    ~Instance() override = default;

    std::string toString() const override;

    void commit() override;

    affine3f xfm();

    // Data //

    affine3f instanceXfm;
    affine3f rcpXfm;

    Ref<Group> group;
  };

  OSPTYPEFOR_SPECIALIZATION(Instance *, OSP_INSTANCE);

  // Inlined definitions //////////////////////////////////////////////////////

  inline affine3f Instance::xfm()
  {
    return instanceXfm;
  }

}  // namespace ospray
