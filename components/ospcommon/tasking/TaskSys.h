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
// stl
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace ospcommon {
  namespace tasking {

    struct OSPCOMMON_INTERFACE __aligned(64) Task
    {
      Task(bool isDynamicallyAllocated = false);
      virtual ~Task() = default;

      // ------------------------------------------------------------------
      // interface for scheduling a new task into the task system
      // ------------------------------------------------------------------

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

      /*! the order in the queue that this job will get scheduled when
       *  activated */
      ScheduleOrder order;

      //! schedule the given task with the given number of sub-jobs.
      void schedule(int numJobs, ScheduleOrder order=BACK_OF_QUEUE);

      //! same as schedule(), but also wait for all jobs to complete
      void scheduleAndWait(int numJobs, ScheduleOrder order=BACK_OF_QUEUE);

      //! wait for the task to complete, optionally (by default) helping
      //! to actually work on completing this task.
      void wait();

    private:

      //! Allow tasking system backend to access all parts of the class, but
      //! prevent users from using data which is an implementation detail of the
      //! task
      friend struct TaskSys;

      // ------------------------------------------------------------------
      // callback used to define what the task is doing
      // ------------------------------------------------------------------

      virtual void run(int jobID) = 0;

      // ------------------------------------------------------------------
      // internal data for the tasking systme to manage the task
      // ------------------------------------------------------------------

      //*! work on task until no more useful job available on this task
      void workOnIt();

      //! activate job, and insert into the task system. should never be
      //! called by the user, only by the task(system) whenever the task
      //! is a) scheduled and b) all dependencies are fulfilled
      void activate();

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
      bool dynamicallyAllocated {false};
    };

    // Public interface to the tasking system /////////////////////////////////

    /*! \brief initialize the task system with given number of worker
        tasks.

        numThreads==-1 means 'use all that are available; numThreads=0
        means 'no worker thread, assume that whoever calls wait() will
        do the work */
    void OSPCOMMON_INTERFACE initTaskSystem(int numThreads = -1);

    template <typename TASK_T>
    inline void parallel_for(int nTasks, TASK_T && fcn)
    {
      struct LocalTask : public Task
      {
        const TASK_T &t;
        LocalTask(TASK_T&& fcn) : t(std::forward<TASK_T>(fcn)) {}
        void run(int taskIndex) override { t(taskIndex); }
      };

      LocalTask task(std::forward<TASK_T>(fcn));
      task.scheduleAndWait(nTasks);
    }

    template <typename TASK_T>
    inline void schedule(int nTasks, TASK_T && fcn)
    {
      struct LocalTask : public Task
      {
        TASK_T t;
        LocalTask(TASK_T&& fcn)
          : Task(true), t(std::forward<TASK_T>(fcn)) {}
        void run(int) override { t(); }
      };

      auto *task = LocalTask(std::forward<TASK_T>(fcn));
      task->schedule(1, Task::FRONT_OF_QUEUE);
    }

  } // ::ospcommon::tasking
} // ::ospcommon
