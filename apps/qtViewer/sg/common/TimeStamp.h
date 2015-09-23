// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

namespace ospray {
  namespace sg {
    
    //! \brief Implements an abstraction of Time
    /*! Abstracts the concept of time to be used for time-stamping
      node's last 'lastupdated' and /lastmodified' time stamps */
    struct TimeStamp {
      //! \brief constructor 
      TimeStamp(uint64 t) : t(t) {};
      
      //! \brief returns global time(stamp) at time of calling
      static inline TimeStamp now() { return embree::__rdtsc(); }

      //! \brief Allows ot typecast to a uint64 (so times can be compared)
      inline operator uint64 () const { return t; }
      
    private:
      //! \brief the uint64 that stores the time value
      uint64 t;
    };

  } // ::ospray::sg
} // ::ospray


