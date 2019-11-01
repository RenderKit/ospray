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

    class Texture : public ManagedObject<OSPTexture, OSP_TEXTURE>
    {
     public:
      Texture(const std::string &type);
      Texture(const Texture &copy);
      Texture(OSPTexture existing = nullptr);
    };

    static_assert(sizeof(Texture) == sizeof(OSPTexture),
                  "cpp::Texture can't have data members!");

    // Inlined function definitions ///////////////////////////////////////////

    inline Texture::Texture(const std::string &type)
    {
      ospObject = ospNewTexture(type.c_str());
    }

    inline Texture::Texture(const Texture &copy)
        : ManagedObject<OSPTexture, OSP_TEXTURE>(copy.handle())
    {
      ospRetain(copy.handle());
    }

    inline Texture::Texture(OSPTexture existing)
        : ManagedObject<OSPTexture, OSP_TEXTURE>(existing)
    {
    }

  }  // namespace cpp

  OSPTYPEFOR_SPECIALIZATION(cpp::Texture, OSP_TEXTURE);

}  // namespace ospray
