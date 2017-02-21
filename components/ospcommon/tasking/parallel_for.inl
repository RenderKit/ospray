// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#  include "ospcommon/tasking/TaskSys.h"
#elif defined(OSPRAY_TASKING_LIBDISPATCH)
#  include "dispatch/dispatch.h"
template <typename TASK_T>
inline void callFcn_T(void *_task, size_t taskIndex)
{
  auto &task = *((TASK_T*)_task);
  task(taskIndex);
}
#endif

namespace ospcommon {

  template<typename TASK_T>
  inline void parallel_for_impl(int nTasks, TASK_T&& fcn)
  {
#ifdef OSPRAY_TASKING_TBB
    tbb::parallel_for(0, nTasks, 1, std::forward<TASK_T>(fcn));
#elif defined(OSPRAY_TASKING_CILK)
    cilk_for (int taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
      fcn(taskIndex);
    }
#elif defined(OSPRAY_TASKING_OMP)
# pragma omp parallel for schedule(dynamic)
    for (int taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
      fcn(taskIndex);
    }
#elif defined(OSPRAY_TASKING_INTERNAL)
    struct LocalTask : public Task
    {
      const TASK_T &t;
      LocalTask(TASK_T&& fcn) : t(std::forward<TASK_T>(fcn)) {}
      void run(int taskIndex) override { t(taskIndex); }
    };

    LocalTask task(fcn);
    task.scheduleAndWait(nTasks);
#elif defined(OSPRAY_TASKING_LIBDISPATCH)
    dispatch_apply_f(nTasks,
                     dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,
                                               0),
                     &fcn,
                     &callFcn_T<TASK_T>);
#else // Debug (no tasking system)
    for (int taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
      fcn(taskIndex);
    }
#endif
  }

} //::ospcommon
