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

#include "../platform.h"
#include "../sysinfo.h"
#include "../RefCount.h"
// std
#include <mutex>
#include <condition_variable>

namespace ospcommon {

  struct OSPCOMMON_INTERFACE __aligned(64) Task : public RefCount {

    Task(const char *name = "no name");
    virtual ~Task();

    // ------------------------------------------------------------------
    // interface for scheduling a new task into the task system
    // ------------------------------------------------------------------

    typedef enum {
      /*! schedule job to the END of the job queue, meaning it'll get
          pulled only after all the ones already in the queue */
      BACK_OF_QUEUE,
      /*! schedule job to the FRONT of the queue, meaning it'll likely
          get processed even before other jobs that are already in the
          queue */
      FRONT_OF_QUEUE
    } ScheduleOrder;

    /*! the order in the queue that this job will get scheduled when
     *  activated */
    ScheduleOrder order;

    //! schedule the given task with the given number of sub-jobs.
    void schedule(size_t numJobs, ScheduleOrder order=BACK_OF_QUEUE);

    //! same as schedule(), but also wait for all jobs to complete
    void scheduleAndWait(size_t numJobs, ScheduleOrder order=BACK_OF_QUEUE);

    //! wait for the task to complete, optionally (by default) helping
    //! to actually work on completing this task.
    void wait(bool workOnIt = true);

    //! get name of the task (useful for debugging)
    const char *getName();

    /*! \brief initialize the task system with given number of worker
        tasks.

        numThreads==-1 means 'use all that are available; numThreads=0
        means 'no worker thread, assume that whoever calls wait() will
        do the work */
    static void initTaskSystem(const size_t numThreads);

  private:

    //! Allow tasking system backend to access all parts of the class, but
    //! prevent users from using data which is an implementation detail of the
    //! task
    friend struct TaskSys;

    // ------------------------------------------------------------------
    // callback used to define what the task is doing
    // ------------------------------------------------------------------

    virtual void run(size_t jobID) = 0;

    // ------------------------------------------------------------------
    // internal data for the tasking systme to manage the task
    // ------------------------------------------------------------------

    //*! work on task until no more useful job available on this task
    void workOnIt();

    //! activate job, and insert into the task system. should never be
    //! called by the user, only by the task(system) whenever the task
    //! is a) scheduled and b) all dependencies are fulfilled
    void activate();

    __aligned(64) std::atomic_int numJobsCompleted;
    __aligned(64) std::atomic_int numJobsStarted;
    size_t    numJobsInTask;

    typedef enum { INITIALIZING, SCHEDULED, ACTIVE, COMPLETED } Status;
    std::mutex __aligned(64) mutex;
    Status volatile __aligned(64) status;
    std::atomic_int __aligned(64) numMissingDependencies;
    std::condition_variable __aligned(64) allDependenciesFulfilledCond;
    std::condition_variable __aligned(64) allJobsCompletedCond;

    __aligned(64) Task *volatile next;
    const char *name;
  };

// Inlined function definitions ///////////////////////////////////////////////

  __forceinline Task::Task(const char *name)
    : numJobsCompleted(),
      numJobsStarted(),
      numJobsInTask(0),
      status(Task::INITIALIZING),
      numMissingDependencies(),
      name(name)
  {
  }

  __forceinline Task::~Task()
  {
  }

  __forceinline const char *Task::getName()
  {
    return name;
  }

} // ::ospcommon
