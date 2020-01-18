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

#include "ManagedObject.h"

namespace ospray {
  namespace cpp {

    class Group : public ManagedObject<OSPGroup, OSP_GROUP>
    {
     public:
      Group();
      Group(const Group &copy);
      Group(OSPGroup existing);
    };

    static_assert(sizeof(Group) == sizeof(OSPGroup),
                  "cpp::Group can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline Group::Group()
    {
      ospObject = ospNewGroup();
    }

    inline Group::Group(const Group &copy)
        : ManagedObject<OSPGroup, OSP_GROUP>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline Group::Group(OSPGroup existing)
        : ManagedObject<OSPGroup, OSP_GROUP>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::Group, OSP_GROUP);

}  // namespace ospray
