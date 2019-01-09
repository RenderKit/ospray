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

#include "../TypeTraits.h"
#include "detail/parallel_for.inl"

#include <algorithm>

namespace ospcommon {
  namespace tasking {

    // NOTE(jda) - This abstraction wraps "fork-join" parallelism, with an
    //             implied synchronizsation after all of the tasks have run.
    template<typename INDEX_T, typename TASK_T>
    inline void parallel_for(INDEX_T nTasks, TASK_T&& fcn)
    {
      using namespace traits;
      static_assert(is_valid_index<INDEX_T>::value,
                    "ospcommon::tasking::parallel_for() requires the type"
                    " INDEX_T to be unsigned char, short, int, uint, long,"
                    " long long, or size_t.");

      static_assert(has_operator_method_matching_param<TASK_T, INDEX_T>::value,
                    "ospcommon::tasking::parallel_for() requires the "
                    "implementation of method "
                    "'void TASK_T::operator(P taskIndex), where P is of "
                    "type INDEX_T [first parameter of parallel_for()].");

      detail::parallel_for_impl(nTasks, std::forward<TASK_T>(fcn));
    }

    // NOTE(jda) - Allow serial version of parallel_for() without the need to
    //             change the entire tasking system backend
    template<typename INDEX_T, typename TASK_T>
    inline void serial_for(INDEX_T nTasks, const TASK_T& fcn)
    {
      using namespace traits;
      static_assert(is_valid_index<INDEX_T>::value,
                    "ospcommon::tasking::serial_for() requires the type"
                    " INDEX_T to be unsigned char, short, int, uint, long,"
                    " or size_t.");

      static_assert(has_operator_method_matching_param<TASK_T, INDEX_T>::value,
                    "ospcommon::tasking::serial_for() requires the "
                    "implementation of method "
                    "'void TASK_T::operator(P taskIndex), where P is of "
                    "type INDEX_T [first parameter of serial_for()].");

      for (INDEX_T taskIndex = 0; taskIndex < nTasks; ++taskIndex)
        fcn(taskIndex);
    }



    /* NOTE(iw) - This abstraction extends the 'parallel_for' to mixed
       parallel/serial: we logically view the domain of N input tasks
       as grouped into roundUp(N/M) blocks of (at most) M items each;
       then 'itearte over the N/M blocks in parallel, and process each
       block serailly */
    template<int BLOCK_SIZE, typename INDEX_T, typename TASK_T>
    inline void parallel_in_blocks_of(INDEX_T nTasks, TASK_T&& fcn)
    {
      using namespace traits;
      static_assert(is_valid_index<INDEX_T>::value,
                    "ospcommon::tasking::parallel_for() requires the type"
                    " INDEX_T to be unsigned char, short, int, uint, long,"
                    " or size_t.");

      // // iw - TODO: fix this
      // static_assert(has_operator_method_matching_param<TASK_T, INDEX_T>::value,
      //               "ospcommon::tasking::parallel_for() requires the "
      //               "implementation of method "
      //               "'void TASK_T::operator(P taskIndex), where P is of "
      //               "type INDEX_T [first parameter of parallel_for()].");

      INDEX_T numBlocks = (nTasks+BLOCK_SIZE-1)/BLOCK_SIZE;
      parallel_for(numBlocks,[&](INDEX_T blockID){
          INDEX_T begin = blockID * (INDEX_T)BLOCK_SIZE;
          INDEX_T end   = std::min(begin+(INDEX_T)BLOCK_SIZE,nTasks);
          fcn(begin,end);
        });
    }

  } // ::ospcommon::tasking
} //::ospcommon
