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

#include "../../common.h"
// enkiTS
#include "enkiTS/TaskScheduler.h"

namespace ospcommon {
  namespace tasking {
    namespace detail {

      // Public interface to the tasking system ///////////////////////////////

      using Task = enki::ITaskSet;

      void OSPCOMMON_INTERFACE initTaskSystemInternal(int numThreads = -1);

      int OSPCOMMON_INTERFACE numThreadsTaskSystemInternal();

      void OSPCOMMON_INTERFACE scheduleTaskInternal(Task *task);

      void OSPCOMMON_INTERFACE waitInternal(Task *task);

      template <typename TASK_T>
      inline void parallel_for_internal(int nTasks, TASK_T &&fcn)
      {
        struct LocalTask : public Task
        {
          const TASK_T &t;
          LocalTask(int nunTasks, TASK_T &&fcn)

              : Task(nunTasks), t(std::forward<TASK_T>(fcn))
          {
          }

          ~LocalTask() override = default;

          void ExecuteRange(enki::TaskSetPartition tp, uint32_t) override
          {
            for (auto i = tp.start; i < tp.end; ++i)
              t(i);
          }
        };

        LocalTask task(nTasks, std::forward<TASK_T>(fcn));
        scheduleTaskInternal(&task);
        waitInternal(&task);
      }

      template <typename TASK_T>
      inline void schedule_internal(TASK_T &&fcn)
      {
        struct LocalTask : public Task
        {
          TASK_T t;

          LocalTask(TASK_T &&fcn) : Task(1), t(std::forward<TASK_T>(fcn)) {}

          ~LocalTask() override = default;

          void ExecuteRange(enki::TaskSetPartition, uint32_t) override
          {
            t();
            delete this;
          }
        };

        auto *task = new LocalTask(std::forward<TASK_T>(fcn));
        scheduleTaskInternal(task);
      }

    }  // namespace detail
  }    // namespace tasking
}  // namespace ospcommon
