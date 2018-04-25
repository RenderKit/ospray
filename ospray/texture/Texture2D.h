// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "common/Managed.h"
#include "ospray/OSPTexture.h"

namespace ospray {

  /*! \brief A Texture defined through a 2D Image. */
  struct OSPRAY_SDK_INTERFACE Texture2D : public ManagedObject
  {
    virtual ~Texture2D() override;

    virtual std::string toString() const override;

    /*! \brief creates a Texture2D object with the given parameter */
    static Texture2D *createTexture(const vec2i &size, const OSPTextureFormat,
                                    void *data, const int flags);

    vec2i size;
    OSPTextureFormat type;
    void *data;
    int flags;
  };

} // ::ospray
