// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "ospray/common/OSPCommon.h"

namespace ospray {

  struct __aligned(64) Task : public RefCount {
    // typedef enum { FRONT, BACK } ScheduleOrder;

    // ------------------------------------------------------------------
    // callbacks used to define what the task is doing
    // ------------------------------------------------------------------
    Task(const char *name = "no name");
    virtual void run(size_t jobID) = 0;
    virtual void finish();
    virtual ~Task();
    // ------------------------------------------------------------------
    // interface for scheduling a new task into the task system
    // ------------------------------------------------------------------

    //! add a new dependecy: task cannot become active until this depdency has
    //! completed
    void addDependency(Task *dependency);

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

    //! schedule the given task with the given number of
    //! sub-jobs. . if the task has dependencies, it may not be
    //! immeately active.
    void schedule(size_t numJobs, ScheduleOrder order=BACK_OF_QUEUE);

    void scheduleAndWait(size_t numJobs, ScheduleOrder order=BACK_OF_QUEUE);

    //! wait for the task to complete, optionally (by default) helping
    //! to actually work on completing this task.
    void wait(bool workOnIt = true);

    //*! work on task until no more useful job available on this task
    void workOnIt();

    //! activate job, and insert into the task system. should never be
    //! called by the user, only by the task(system) whenever the task
    //! is a) scheduled and b) all dependencies are fulfilled
    void activate();

    __aligned(64) AtomicInt numJobsCompleted;
    __aligned(64) AtomicInt numJobsStarted;
    size_t    numJobsInTask;

    typedef enum { INITIALIZING, SCHEDULED, ACTIVE, COMPLETED } Status;
    Mutex __aligned(64) mutex;
    Status volatile __aligned(64) status;
    AtomicInt __aligned(64) numMissingDependencies;
    Condition __aligned(64) allDependenciesFulfilledCond;
    Condition __aligned(64) allJobsCompletedCond;

    __aligned(64) Task *volatile next;
    const char *name;

    /*! \brief initialize the task system with given number of worker
        tasks.

        numThreads==-1 means 'use all that are available; numThreads=0
        means 'no worker thread, assume that whoever calls wait() will
        do the work */
    static void initTaskSystem(const size_t numThreads);
  };

// Inlined function definitions ///////////////////////////////////////////////

  __forceinline Task::Task(const char *name)
    : status(Task::INITIALIZING),
      name(name),
      numJobsCompleted(),
      numJobsStarted(),
      numMissingDependencies(),
      numJobsInTask(0)
  {
  }

  __forceinline Task::~Task()
  {
  }

  __forceinline void Task::finish()
  {
  }
}//namespace ospray
