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

#include "common.h"

namespace ospcommon {

  /*! flag that will glbally turn off all microbenches */
// #define ALLOW_MICRO_BENCHES 1

  struct MicroBench {
#if ALLOW_MICRO_BENCHES
    template<typename Lambda>
    MicroBench(const char *name, const Lambda &func) {
      static size_t numTimesCalled = 0;

      ++numTimesCalled;
      static size_t t_first = 0;
      static size_t t_in    = 0;
      static double s_first;
      size_t t_enter = __rdtsc();
      if (t_first == 0) { t_first = t_enter; s_first = getSysTime(); }
      func();
      size_t t_now = __rdtsc();
      size_t t_leave = t_now;
      size_t t_this  = t_leave - t_enter;
      t_in += t_this;
        
      static size_t t_lastPing = t_first;
      if (t_now-t_lastPing > 10000000000ULL) {
        size_t t_total = t_leave - t_first;
        double s_now = getSysTime();
        printf("pct time in %s: %.2f (%.1f secs in; num times called %li)\n",
               name,t_in*100.f/t_total,s_now-s_first,numTimesCalled);
        t_lastPing = t_now;
      }
    }
#else
    template<typename Lambda>
    inline MicroBench(const char *, const Lambda &func) {
      func();
    }
#endif

  };

} // ::ospray
