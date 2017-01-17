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

#include "../common.h"

#include "TaskingTypeTraits.h"

#ifdef OSPRAY_TASKING_TBB
#  include <tbb/task.h>
#elif defined(OSPRAY_TASKING_CILK)
#  include <cilk/cilk.h>
#elif defined(OSPRAY_TASKING_INTERNAL)
#  include "TaskSys.h"
#endif

namespace ospcommon {

  // NOTE(jda) - This abstraction takes a lambda which should take captured
  //             variables by *value* to ensure no captured references race
  //             with the task itself.

  // NOTE(jda) - No priority is associated with this call, but could be added
  //             later with a hint enum, using a default value for the priority
  //             to not require specifying it.
  template<typename TASK_T>
  inline void async(TASK_T&& fcn)
  {
    static_assert(has_operator_method<TASK_T>::value,
                  "ospray::async() requires the implementation of method "
                  "'void TASK_T::operator()'.");

#ifdef OSPRAY_TASKING_TBB
    struct LocalTBBTask : public tbb::task
    {
      TASK_T func;
      tbb::task* execute() override { func(); return nullptr; }
      LocalTBBTask(TASK_T&& f) : func(std::forward<TASK_T>(f)) {}
    };

    auto *tbb_node =
        new(tbb::task::allocate_root())LocalTBBTask(std::forward<TASK_T>(fcn));
    tbb::task::enqueue(*tbb_node);
#elif defined(OSPRAY_TASKING_CILK)
    cilk_spawn fcn();
#elif defined(OSPRAY_TASKING_INTERNAL)
    struct LocalTask : public Task {
      TASK_T t;
      LocalTask(TASK_T&& fcn)
        : Task("LocalTask"), t(std::forward<TASK_T>(fcn)) {}
      void run(size_t) override { t(); }
    };

    Ref<LocalTask> task = new LocalTask(std::forward<TASK_T>(fcn));
    task->schedule(1, Task::FRONT_OF_QUEUE);
#else// OpenMP or Debug --> synchronous!
    fcn();
#endif
  }

} // ::ospcommon
