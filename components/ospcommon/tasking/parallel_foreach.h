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

#pragma once

#include "parallel_for.h"

#include <array>
#include <vector>

namespace ospcommon {
  namespace tasking {

    template<typename ELEMENT_T, typename TASK_T>
    inline void parallel_foreach(std::vector<ELEMENT_T> &v, TASK_T&& f)
    {
      parallel_for(v.size(), [&](size_t i){
        f(v[i]);
      });
    }

    template<typename ELEMENT_T, typename TASK_T>
    inline void parallel_foreach(const std::vector<ELEMENT_T> &v, TASK_T&& f)
    {
      parallel_for(v.size(), [&](size_t i){
        f(v[i]);
      });
    }

    template<typename ELEMENT_T, size_t N, typename TASK_T>
    inline void parallel_foreach(std::array<ELEMENT_T, N> &v, TASK_T&& f)
    {
      parallel_for(N, [&](size_t i){
        f(v[i]);
      });
    }

    template<typename ELEMENT_T, size_t N, typename TASK_T>
    inline void parallel_foreach(const std::array<ELEMENT_T, N> &v, TASK_T&& f)
    {
      parallel_for(N, [&](size_t i){
        f(v[i]);
      });
    }

  } // ::ospcommon::tasking
} //::ospcommon
