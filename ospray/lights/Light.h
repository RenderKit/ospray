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

namespace ospray {

  //! Base class for Light objects
  struct OSPRAY_SDK_INTERFACE Light : public ManagedObject
  {
    virtual ~Light() override = default;

    //! Create a light of the given type
    static Light *createInstance(const char *type);

    //! Copy understood parameters into class members
    virtual void commit() override;

    //! toString is used to aid in printf debugging
    virtual std::string toString() const override;

    bool isVisible; //!< either directly in camera, or via a straight path
  };

#define OSP_REGISTER_LIGHT(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(::ospray::Light, light, InternalClass, external_name)

} // ::ospray
