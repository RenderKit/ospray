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
  namespace cpp {

    class Material : public ManagedObject_T<OSPMaterial>
    {
    public:

      Material() = default;
      Material(const std::string &renderer_type, const std::string &mat_type);
      Material(const Material &copy);
      Material(OSPMaterial existing);
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline Material::Material(const std::string &renderer_type,
                              const std::string &mat_type)
    {
      auto c = ospNewMaterial2(renderer_type.c_str(), mat_type.c_str());
      if (c) {
        ospObject = c;
      } else {
        throw std::runtime_error("Failed to create OSPMaterial (of type '"+renderer_type+"'::'"+mat_type+"')!");
      }
    }

    inline Material::Material(const Material &copy) :
      ManagedObject_T<OSPMaterial>(copy.handle())
    {
    }

    inline Material::Material(OSPMaterial existing) :
      ManagedObject_T<OSPMaterial>(existing)
    {
    }

  }// namespace cpp
}// namespace ospray
