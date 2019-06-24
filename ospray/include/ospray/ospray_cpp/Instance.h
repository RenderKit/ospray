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

#include <ospray/ospray_cpp/GeometricModel.h>
#include <ospray/ospray_cpp/ManagedObject.h>
#include <ospray/ospray_cpp/VolumetricModel.h>

namespace ospray {
  namespace cpp {

    class Instance : public ManagedObject_T<OSPInstance>
    {
     public:
      Instance();
      Instance(const Instance &copy);
      Instance(OSPInstance existing);
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline Instance::Instance()
    {
      OSPInstance c = ospNewInstance();
      if (c) {
        ospObject = c;
      } else {
        throw std::runtime_error("Failed to create OSPInstance!");
      }
    }

    inline Instance::Instance(const Instance &copy)
        : ManagedObject_T<OSPInstance>(copy.handle())
    {
    }

    inline Instance::Instance(OSPInstance existing)
        : ManagedObject_T<OSPInstance>(existing)
    {
    }

  }  // namespace cpp
}  // namespace ospray
