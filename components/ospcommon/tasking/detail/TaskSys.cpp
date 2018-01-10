// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "../../platform.h"
//stl
#include <thread>
#include <vector>

namespace ospcommon {
  namespace tasking {
    namespace detail {

      struct TaskSys
      {
        TaskSys();
        ~TaskSys();

        void  init(int maxNumRenderTasks);
        void  createWorkerThreads(int numThreads);
        Task *getNextActiveTask();
        void  shutdownWorkerThreads();

        // Data members //

        bool initialized {false};
        bool running {false};

        static TaskSys global;

        //! Queue of tasks that have ALREADY been acitvated, which are ready
        __aligned(64) Task *volatile activeListFirst {nullptr};
        __aligned(64) Task *volatile activeListLast  {nullptr};

        std::mutex __aligned(64) mutex;
        std::condition_variable __aligned(64) tasksAvailable;

        std::vector<std::thread> threads;
      };

      // Task definitions /////////////////////////////////////////////////////

      Task::Task(bool needsToBeDeleted)
      : numJobsCompleted(),
        numJobsStarted(),
        willNeedToBeDeleted(needsToBeDeleted)
      {
      }

      void Task::workOnIt()
      {
        int myCompleted = 0;

        while (true) {
          const int thisJobID = numJobsStarted++;

          if (thisJobID >= numJobsInTask)
            break;

          run(thisJobID);
          ++myCompleted;
        }

        if (myCompleted != 0) {
          const int nowCompleted = (numJobsCompleted += myCompleted);
          if (nowCompleted == numJobsInTask) {
            {
              SCOPED_LOCK(mutex);
              status = Task::COMPLETED;
              allJobsCompletedCond.notify_all();
            }
            if (willNeedToBeDeleted)
              delete this;
          }
        }
      }

      void Task::wait()
      {
        if (status == Task::COMPLETED)
          return;

        this->workOnIt();

        std::unique_lock<std::mutex> lock(mutex);
        allJobsCompletedCond.wait(lock, [&](){return status==Task::COMPLETED;});
      }

      // TaskSys definitions //////////////////////////////////////////////////

      TaskSys __aligned(64) TaskSys::global;

      inline TaskSys::TaskSys()
      {
    #ifdef OSPRAY_TASKING_INTERNAL
        init(-1);
    #endif
      }

      inline TaskSys::~TaskSys()
      {
        shutdownWorkerThreads();
      }

      inline void TaskSys::init(int numThreads)
      {
        if (initialized)
          shutdownWorkerThreads();

        initialized = true;
        running     = true;

        if (numThreads >= 0) {
          numThreads = std::min(numThreads,
                                (int)std::thread::hardware_concurrency());
        } else {
          numThreads = (int)std::thread::hardware_concurrency();
        }

        createWorkerThreads(numThreads);
      }

      inline void TaskSys::createWorkerThreads(int numThreads)
      {
        for (int t = 1; t < numThreads; t++) {
          threads.emplace_back([&](){
            while (true) {
              Task *task = getNextActiveTask();
              if (!running)
                break;
              if(task == nullptr)
                continue;
              task->workOnIt();
            }
          });
        }
      }

      inline Task *TaskSys::getNextActiveTask()
      {
        while (true) {
          std::unique_lock<std::mutex> lock(mutex);
          tasksAvailable.wait(lock, [&](){
            return !(activeListFirst == nullptr && running);
          });

          if (!running)
            return nullptr;

          Task *front = activeListFirst;
          if (front->numJobsStarted >= front->numJobsInTask) {
            if (activeListFirst == activeListLast)
              activeListFirst = activeListLast = nullptr;
            else
              activeListFirst = activeListFirst->next;

            continue;
          }

          return front;
        }
      }

      inline void TaskSys::shutdownWorkerThreads()
      {
        running = false;
        tasksAvailable.notify_all();
        for (auto &thread : threads)
          thread.join();
        threads.clear();
      }

      // Interface definitions ////////////////////////////////////////////////

      void initTaskSystemInternal(int maxNumRenderTasks)
      {
        TaskSys::global.init(maxNumRenderTasks);
      }

      int OSPCOMMON_INTERFACE numThreadsTaskSystemInternal()
      {
        return static_cast<int>(TaskSys::global.threads.size());
      }

      void scheduleTaskInternal(Task *task,
                                int numJobs,
                                ScheduleOrder order)
      {
        task->numJobsInTask = numJobs;
        task->status = Task::SCHEDULED;

        SCOPED_LOCK(TaskSys::global.mutex);
        bool wasEmpty = TaskSys::global.activeListFirst == nullptr;
        if (wasEmpty) {
          TaskSys::global.activeListFirst=TaskSys::global.activeListLast=task;
          task->next = nullptr;
          TaskSys::global.tasksAvailable.notify_all();
        } else {
          if (order == BACK_OF_QUEUE) {
            task->next = nullptr;
            TaskSys::global.activeListLast->next = task;
            TaskSys::global.activeListLast = task;
          } else {
            task->next = TaskSys::global.activeListFirst;
            TaskSys::global.activeListFirst = task;
          }
        }

        task->status = Task::ACTIVE;
      }

    } // ::ospcommon::tasking::detail
  } // ::ospcommon::tasking
} // ::ospcommon
