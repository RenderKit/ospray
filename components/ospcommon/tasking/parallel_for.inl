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
#endif

namespace ospcommon {

  template<typename TASK_T>
  inline void parallel_for_impl(int nTasks, TASK_T&& fcn)
  {
#ifdef OSPRAY_TASKING_TBB
    tbb::parallel_for(0, nTasks, 1, fcn);
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
    struct LocalTask : public Task {
      const TASK_T &t;
      LocalTask(const TASK_T& fcn) : Task("LocalTask"), t(fcn) {}
      void run(size_t taskIndex) override { t(taskIndex); }
    };

    Ref<LocalTask> task = new LocalTask(fcn);
    task->scheduleAndWait(nTasks);
#else // Debug (no tasking system)
    for (int taskIndex = 0; taskIndex < nTasks; ++taskIndex) {
      fcn(taskIndex);
    }
#endif
  }

} //::ospcommon
