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

#include "Managed.h"

namespace ospray {
  /*! \brief implements the basic abstraction for anything that is a 'texture'.

   */
  struct OSPRAY_SDK_INTERFACE Texture : public ManagedObject
  {
    virtual ~Texture() override = default;
    //! \brief common function to help printf-debugging
    /*! Every derived class should override this! */
    virtual std::string toString() const { return "ospray::Texture"; }
  };
}
