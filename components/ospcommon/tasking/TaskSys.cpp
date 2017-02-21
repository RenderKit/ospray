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

#include "TaskSys.h"
//ospray
#include "../sysinfo.h"
#include "../thread.h"
//stl
#include <algorithm>
#include <thread>
#include <vector>

namespace ospcommon {
  
  struct TaskSys
  {
    ~TaskSys();

    void      init(size_t maxNumRenderTasks);
    Ref<Task> getNextActiveTask();
    void      shutdownWorkerThreads();

    // Data members //

    bool initialized {false};
    bool running {false};

    static TaskSys global;

    //! Queue of tasks that have ALREADY been acitvated, which are ready to run
    __aligned(64) Task *volatile activeListFirst {nullptr};
    __aligned(64) Task *volatile activeListLast {nullptr};

    std::mutex __aligned(64) mutex;
    std::condition_variable __aligned(64) tasksAvailable;

    std::vector<std::thread> threads;
  };
  
  TaskSys __aligned(64) TaskSys::global;

  inline TaskSys::~TaskSys()
  {
    shutdownWorkerThreads();
  }

  inline void Task::workOnIt() 
  {
    size_t myCompleted = 0;
    while (true) {
      const size_t thisJobID = numJobsStarted++;

      if (thisJobID >= numJobsInTask) 
        break;
      
      run(thisJobID);
      ++myCompleted;
    }

    if (myCompleted != 0) {
      const size_t nowCompleted = (numJobsCompleted += myCompleted);
      if (nowCompleted == numJobsInTask) {
        SCOPED_LOCK(mutex);
        status = Task::COMPLETED;
        allJobsCompletedCond.notify_all();
      }
    }
  }
  
  void Task::wait(bool workOnIt)
  {
    if (status == Task::COMPLETED)
      return;

    if (workOnIt)
      this->workOnIt();

    std::unique_lock<std::mutex> lock(mutex);
    allJobsCompletedCond.wait(lock, [&](){return status == Task::COMPLETED;});
  }

  void Task::scheduleAndWait(size_t numJobs, ScheduleOrder order)
  {
    schedule(numJobs,order);
    wait();
  }

  inline Ref<Task> TaskSys::getNextActiveTask()
  {
    while (true) {
      std::unique_lock<std::mutex> lock(mutex);
      tasksAvailable.wait(lock, [&](){
        return !(activeListFirst == nullptr && running);
      });

      if (!running)
        return nullptr;

      Ref<Task> front = activeListFirst;
      if (front->numJobsStarted >= int(front->numJobsInTask)) {

        if (activeListFirst == activeListLast)
          activeListFirst = activeListLast = nullptr;
        else
          activeListFirst = activeListFirst->next;

        continue;
      }

      return front;
    }
  }

  void TaskSys::shutdownWorkerThreads()
  {
    running = false;
    tasksAvailable.notify_all();
    for (auto &thread : threads)
      thread.join();
    threads.clear();
  }
    
  void Task::initTaskSystem(const size_t maxNumRenderTasks)
  {
    TaskSys::global.init(maxNumRenderTasks);
  }

  void Task::schedule(size_t numJobs, ScheduleOrder order)
  {
    refInc();
    this->order = order;
    numJobsInTask = numJobs;
    status = Task::SCHEDULED;
    activate();
  }

  inline void Task::activate()
  {
    if (!TaskSys::global.initialized)
      throw std::runtime_error("TASK SYSTEM NOT YET INITIALIZED");
    {
      SCOPED_LOCK(TaskSys::global.mutex);
      bool wasEmpty = TaskSys::global.activeListFirst == nullptr;
      if (wasEmpty) {
        TaskSys::global.activeListFirst = TaskSys::global.activeListLast = this;
        this->next = nullptr;
        TaskSys::global.tasksAvailable.notify_all();
      } else {
        if (order == Task::BACK_OF_QUEUE) {
          this->next = nullptr;
          TaskSys::global.activeListLast->next = this;
          TaskSys::global.activeListLast = this;
        } else {
          this->next = TaskSys::global.activeListFirst;
          TaskSys::global.activeListFirst = this;
        }
      }
      status = Task::ACTIVE;
    }
  }

  void TaskSys::init(size_t numThreads)
  {
    if (initialized)
      shutdownWorkerThreads();

    initialized = true;
    running     = true;

    if (numThreads >= 0) {
      numThreads = std::min(numThreads,
                            (size_t)std::thread::hardware_concurrency());
    }

    /* generate all threads */
    for (size_t t = 1; t < numThreads; t++) {
      threads.emplace_back([&](){
        while (true) {
          Ref<Task> task = getNextActiveTask();

          if (!running)
            break;

          task->workOnIt();
        }
      });
    }
  }
  
} // ::ospcommon
