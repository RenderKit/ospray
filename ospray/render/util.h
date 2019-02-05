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

/*! \file render/util.h Defines some utility functions shaared by different shading codes */

#include "common/OSPCommon.h"

namespace ospray {

  //! generates a "random" color from an int.
  inline vec3f makeRandomColor(const int i)
  {
    const int mx = 13*17*43;
    const int my = 11*29;
    const int mz = 7*23*63;
    const uint32 g = (i * (3*5*127)+12312314);
    return vec3f((g % mx)*(1.f/(mx-1)),
                 (g % my)*(1.f/(my-1)),
                 (g % mz)*(1.f/(mz-1)));
  }

} // ::ospray
