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

#include <utility>

#ifdef OSPRAY_TASKING_TBB
#  include <tbb/parallel_for.h>
#elif defined(OSPRAY_TASKING_CILK)
#  include <cilk/cilk.h>
#elif defined(OSPRAY_TASKING_INTERNAL)
#  include "TaskSys.h"
#elif defined(OSPRAY_TASKING_LIBDISPATCH)
#  include "dispatch/dispatch.h"
#endif

namespace ospcommon {
  namespace tasking {
    namespace detail {

#if defined(OSPRAY_TASKING_LIBDISPATCH)
      template<typename INDEX_T, typename TASK_T>
      inline void callFcn_T(void *_task, INDEX_T taskIndex)
      {
        auto &task = *((TASK_T*)_task);
        task(taskIndex);
      }
#endif

      template<typename INDEX_T, typename TASK_T>
      inline void parallel_for_impl(INDEX_T nTasks, TASK_T&& fcn)
      {
#ifdef OSPRAY_TASKING_TBB
        tbb::parallel_for(INDEX_T(0), nTasks, std::forward<TASK_T>(fcn));
#elif defined(OSPRAY_TASKING_CILK)
        cilk_for (INDEX_T taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
          fcn(taskIndex);
        }
#elif defined(OSPRAY_TASKING_OMP)
#       pragma omp parallel for schedule(dynamic)
        for (INDEX_T taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
          fcn(taskIndex);
        }
#elif defined(OSPRAY_TASKING_INTERNAL)
        detail::parallel_for_internal(nTasks, std::forward<TASK_T>(fcn));
#elif defined(OSPRAY_TASKING_LIBDISPATCH)
        dispatch_apply_f(nTasks,
                         dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,
                                                   0),
                         &fcn,
                         &callFcn_T<TASK_T>);
#else // Debug (no tasking system)
        for (INDEX_T taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
          fcn(taskIndex);
        }
#endif
      }

    } // ::ospcommon::tasking::detail
  } // ::ospcommon::tasking
} // ::ospcommon
