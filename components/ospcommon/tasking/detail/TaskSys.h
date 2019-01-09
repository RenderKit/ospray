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
// stl
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace ospcommon {
  namespace tasking {
    namespace detail {

      enum ScheduleOrder
      {
        /*! schedule job to the END of the job queue, meaning it'll get
            pulled only after all the ones already in the queue */
        BACK_OF_QUEUE,
        /*! schedule job to the FRONT of the queue, meaning it'll likely
            get processed even before other jobs that are already in the
            queue */
        FRONT_OF_QUEUE
      };

      struct OSPCOMMON_INTERFACE __aligned(64) Task
      {
        Task(bool needsToBeDeleted);
        virtual ~Task() = default;

        // ------------------------------------------------------------------
        // interface for scheduling a new task into the task system
        // ------------------------------------------------------------------

        //! wait for the task to complete, optionally (by default) helping
        //! to actually work on completing this task.
        void wait();

        // ------------------------------------------------------------------
        // callback used to define what the task is doing
        // ------------------------------------------------------------------

        virtual void run(int jobID) = 0;

        // ------------------------------------------------------------------
        // internal data for the tasking systme to manage the task
        // ------------------------------------------------------------------

        //! work on task until no more useful job available on this task
        void workOnIt();

        // Data members //

        __aligned(64) std::atomic_int numJobsCompleted;
        __aligned(64) std::atomic_int numJobsStarted;
        int numJobsInTask {0};

        enum Status { INITIALIZING, SCHEDULED, ACTIVE, COMPLETED };
        std::mutex __aligned(64) mutex;
        Status volatile __aligned(64) status {INITIALIZING};
        std::condition_variable __aligned(64) allDependenciesFulfilledCond;
        std::condition_variable __aligned(64) allJobsCompletedCond;

        __aligned(64) Task *volatile next;
        bool willNeedToBeDeleted {true};
      };

      // Public interface to the tasking system /////////////////////////////////

      /*! \brief initialize the task system with given number of worker
          tasks.

          numThreads==-1 means 'use all that are available; numThreads=0
          means 'no worker thread, assume that whoever calls wait() will
          do the work */
      void OSPCOMMON_INTERFACE initTaskSystemInternal(int numThreads = -1);

      int OSPCOMMON_INTERFACE numThreadsTaskSystemInternal();

      //! schedule the given task with the given number of sub-jobs.
      void scheduleTaskInternal(Task *task,
                                int numJobs,
                                ScheduleOrder order = BACK_OF_QUEUE);

      template <typename TASK_T>
      inline void parallel_for_internal(int nTasks, TASK_T && fcn)
      {
        struct LocalTask : public Task
        {
          const TASK_T &t;
          LocalTask(TASK_T&& fcn) : Task(false), t(std::forward<TASK_T>(fcn)) {}
          void run(int taskIndex) override { t(taskIndex); }
        };

        LocalTask task(std::forward<TASK_T>(fcn));
        scheduleTaskInternal(&task, nTasks);
        task.wait();
      }

      template <typename TASK_T>
      inline void schedule_internal(TASK_T && fcn)
      {
        struct LocalTask : public Task
        {
          TASK_T t;
          LocalTask(TASK_T&& fcn) : Task(true), t(std::forward<TASK_T>(fcn)) {}
          void run(int) override { t(); }
        };

        auto *task = new LocalTask(std::forward<TASK_T>(fcn));
        scheduleTaskInternal(task, 1, FRONT_OF_QUEUE);
      }

    } // ::ospcommon::tasking::detail
  } // ::ospcommon::tasking
} // ::ospcommon
