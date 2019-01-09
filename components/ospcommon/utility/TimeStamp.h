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

#include "../common.h"

#include <atomic>

namespace ospcommon {
  namespace utility {

    struct OSPCOMMON_INTERFACE TimeStamp
    {
      TimeStamp() = default;
      TimeStamp(const TimeStamp &);
      TimeStamp(TimeStamp &&);

      TimeStamp &operator=(const TimeStamp &);
      TimeStamp &operator=(TimeStamp &&);

      operator size_t() const;

      void renew();

    private:

      static size_t nextValue();

      // Data members //

      std::atomic<size_t> value {nextValue()};

      //! \brief the uint64_t that stores the time value
      static std::atomic<size_t> global;
    };

  } // ::ospray::sg
} // ::ospray
