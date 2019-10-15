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

    class Material : public ManagedObject<OSPMaterial, OSP_MATERIAL>
    {
     public:
      Material(const std::string &renderer_type, const std::string &mat_type);
      Material(const Material &copy);
      Material(OSPMaterial existing = nullptr);
    };

    static_assert(sizeof(Material) == sizeof(OSPMaterial),
                  "cpp::Material can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline Material::Material(const std::string &renderer_type,
                              const std::string &mat_type)
    {
      ospObject = ospNewMaterial(renderer_type.c_str(), mat_type.c_str());
    }

    inline Material::Material(const Material &copy)
        : ManagedObject<OSPMaterial, OSP_MATERIAL>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline Material::Material(OSPMaterial existing)
        : ManagedObject<OSPMaterial, OSP_MATERIAL>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::Material, OSP_MATERIAL);

}  // namespace ospray
