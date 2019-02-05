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

#include <ospray/ospray_cpp/ManagedObject.h>

namespace ospray {
  namespace cpp    {

    class Light : public ManagedObject_T<OSPLight>
    {
    public:

      Light(const std::string &light_type);
      Light(const Light &copy);
      Light(OSPLight existing);
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline Light::Light(const std::string &light_type)
    {
      auto c = ospNewLight3(light_type.c_str());
      if (c) {
        ospObject = c;
      } else {
        throw std::runtime_error("Failed to create OSPLight (of type '"+light_type+"')!");
      }
    }

    inline Light::Light(const Light &copy) :
      ManagedObject_T<OSPLight>(copy.handle())
    {
    }

    inline Light::Light(OSPLight existing) :
      ManagedObject_T<OSPLight>(existing)
    {
    }

  }// namespace cpp
}// namespace ospray
