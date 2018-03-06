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

#include "TimeStamp.h"

namespace ospcommon {
  namespace utility {

    std::atomic<size_t> TimeStamp::global {0};

    TimeStamp::TimeStamp(const TimeStamp &other)
    {
      this->value = other.value.load();
    }

    TimeStamp::TimeStamp(TimeStamp &&other)
    {
      this->value = other.value.load();
    }

    TimeStamp &TimeStamp::operator=(const TimeStamp &other)
    {
      this->value = other.value.load();
      return *this;
    }

    TimeStamp &TimeStamp::operator=(TimeStamp &&other)
    {
      this->value = other.value.load();
      return *this;
    }

    TimeStamp::operator size_t() const
    {
      return value;
    }

    void TimeStamp::renew()
    {
      value = nextValue();
    }

    size_t TimeStamp::nextValue()
    {
      return global++;
    }

  } // ::ospray::sg
} // ::ospray
