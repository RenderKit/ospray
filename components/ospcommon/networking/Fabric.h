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

#include <cstdlib>
#include "../common.h"

namespace ospcommon {
  namespace networking {

    /*! abstraction for a physical fabric that can transmit data -
      sockets, mpi, etc */
    struct OSPCOMMON_INTERFACE Fabric
    {
      virtual ~Fabric() = default;

      /*! send exact number of bytes - the fabric can do that through
        multiple smaller messages, but all bytes have to be
        delivered */
      virtual void send(const void *mem, size_t s) = 0;

      /*! receive some block of data - whatever the sender has sent -
        and give us size and pointer to this data */
      virtual size_t read(void *&mem) = 0;
    };

  } // ::ospcommon::networking
} // ::ospcommon
