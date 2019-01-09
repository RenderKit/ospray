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

#include "common/Managed.h"
#include "ospray/OSPTexture.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE Texture : public ManagedObject
  {
    virtual ~Texture() override = default;

    virtual std::string toString() const override;

    /*! \brief creates a Texture2D object with the given parameter */
    static Texture *createInstance(const char *type);
  };

  /*! \brief registers a internal ospray::<ClassName> geometry under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      geometry. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this geometry.
  */
#define OSP_REGISTER_TEXTURE(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(::ospray::Texture, texture, \
                      InternalClass, external_name)

} // ::ospray
