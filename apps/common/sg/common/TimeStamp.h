// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "sg/common/Common.h"
#include "ospcommon/intrinsics.h"

namespace ospray {
  namespace sg {
    
    //! \brief Implements an abstraction of Time
    /*! Abstracts the concept of time to be used for time-stamping
      node's last 'lastupdated' and /lastmodified' time stamps */
    struct TimeStamp {
      //! \brief constructor 
      TimeStamp(uint64_t t) : t(t) {};
      
      //! \brief returns global time(stamp) at time of calling
      static inline TimeStamp now() { return ospcommon::rdtsc(); }

      //! \brief Allows ot typecast to a uint64_t (so times can be compared)
      inline operator uint64_t () const { return t; }
      
    private:
      //! \brief the uint64_t that stores the time value
      uint64_t t;
    };

  } // ::ospray::sg
} // ::ospray


