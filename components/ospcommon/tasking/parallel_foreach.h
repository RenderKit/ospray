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

#include "parallel_for.h"

#include <iterator>

namespace ospcommon {
  namespace tasking {

    template<typename ITERATOR_T, typename TASK_T>
    inline void parallel_foreach(ITERATOR_T begin, ITERATOR_T end, TASK_T&& f)
    {
      using ITERATOR_KIND =
          typename std::iterator_traits<ITERATOR_T>::iterator_category;

      static_assert(std::is_same<ITERATOR_KIND,
                                 std::random_access_iterator_tag>::value,
                    "ospcommon::tasking::parallel_foreach() requires random-"
                    "access iterators!");

      const size_t count = std::distance(begin, end);
      auto *v = &(*begin);

      parallel_for(count, [&](size_t i){ f(v[i]); });
    }

    template<typename CONTAINER_T, typename TASK_T>
    inline void parallel_foreach(CONTAINER_T &&c, TASK_T&& f)
    {
      parallel_foreach(std::begin(c), std::end(c), std::forward<TASK_T>(f));
    }

  } // ::ospcommon::tasking
} //::ospcommon
