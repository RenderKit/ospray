// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include <vector>
#include <algorithm>

// embree
#include "common/sys/sysinfo.h"
#include "common/sys/thread.h"

namespace ospray {
  using std::cout;
  using std::endl;

  struct TaskSys {
    bool initialized;
    bool running;

    void init(size_t maxNumRenderTasks);
    static TaskSys global;
    static void *threadStub(void *);
    inline Task *getNextActiveTask();

    //! queue of tasks that have ALREADY been acitvated, and that are ready to run
    __aligned(64) Task *volatile activeListFirst;
    __aligned(64) Task *volatile activeListLast;

    embree::MutexSys     __aligned(64) mutex;
    Condition __aligned(64) tasksAvailable;

    void threadFunction();

    std::vector<embree::thread_t> threads;

    TaskSys()
      : activeListFirst(NULL), activeListLast(NULL),
        initialized(false), running(false)
    {}

    ~TaskSys();
  };

  size_t numActiveThreads = 0;

  TaskSys __aligned(64) TaskSys::global;

    //! one of our depdencies tells us that he's finished
  void Task::oneDependencyGotCompleted(Task *which)
  {
#if TASKSYS_DEPENDENCIES
    {
      embree::Lock<embree::MutexSys> lock(mutex);
      if (--numMissingDependencies == 0) {
        allDependenciesFulfilledCond.broadcast();
        activate();
      }
    }
#endif
  }

  inline void Task::workOnIt() 
  {
    // work on dependencies, until they are done
    if (numMissingDependencies > 0) {
#if TASKSYS_DEPENDENCIES
      for (size_t i=0;i<dependency.size();i++) {
        dependency[i]->workOnIt();
      }
      {
        embree::Lock<embree::MutexSys> lock(mutex);
        while (numMissingDependencies)
          allDependenciesFulfilledCond.wait(mutex);
      }
#endif
    }
    
    size_t myCompleted = 0;
    while (1) {
      const size_t thisJobID = numJobsStarted++;
      if (thisJobID >= numJobsInTask) 
       break;
      
      run(thisJobID);
      ++myCompleted;
    }

    if (myCompleted != 0) {
      const size_t nowCompleted = (numJobsCompleted += myCompleted); //++numJobsCompleted;
      if (nowCompleted == numJobsInTask) {
        // Yay - I just finished the job, so I get some extra work do do ... just like in real life....
        finish();
        
        {
          embree::Lock<embree::MutexSys> lock(mutex);
          status = Task::COMPLETED;
          allJobsCompletedCond.broadcast();
        }
#if TASKSYS_DEPENDENCIES
        for (int i=0;i<dependent.size();i++)
          dependent[i]->oneDependencyGotCompleted(this);
#endif
      }
    }
  }
  
  void Task::wait(bool workOnIt)
  {
    if (status == Task::COMPLETED) {
      return;
    }

    if (workOnIt) {
      this->workOnIt();
    }

    if (status != COMPLETED) {
      {
        embree::Lock<embree::MutexSys> lock(mutex);
        while (1) {
          const Status status = this->status;
          if (status == Task::COMPLETED)
            break;
          allJobsCompletedCond.wait(mutex);
        }
      }
    }
  }

  void Task::scheduleAndWait(size_t numJobs, ScheduleOrder order)
  {
    schedule(numJobs,order);
    wait();
  }

  inline Task *TaskSys::getNextActiveTask()
  {
    embree::Lock<embree::MutexSys> lock(mutex);
    while (1) {
      while (activeListFirst == NULL && running) {
        tasksAvailable.wait(mutex);
      }

      if (!running) {
        return NULL;
      }

      Task *const front = activeListFirst;
      if (front->numJobsStarted >= front->numJobsInTask) {
        if (activeListFirst == activeListLast) {
          activeListFirst = activeListLast = NULL;
        } else {
          activeListFirst = activeListFirst->next;
        }
        front->refDec();
        continue;
      }
      front->refInc(); // becasue the thread calling us now owns it
      assert(front);
      return front;
    }
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
    if (numMissingDependencies == 0)
      activate();
  }

  inline void Task::activate()
  {
    if (!TaskSys::global.initialized)
      throw std::runtime_error("TASK SYSTEM NOT YET INITIALIZED");
    {
      embree::Lock<embree::MutexSys> lock(TaskSys::global.mutex);
      bool wasEmpty = TaskSys::global.activeListFirst == NULL;
      if (wasEmpty) {
        TaskSys::global.activeListFirst = TaskSys::global.activeListLast = this;
        this->next = NULL;
        TaskSys::global.tasksAvailable.broadcast();
      } else {
        if (order == Task::BACK_OF_QUEUE) {
          this->next = NULL;
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


  void TaskSys::threadFunction()
  {
    while (1) {
      Task *task = getNextActiveTask();
      if (!running) {
        if (task) {
          task->refDec();
        }
        return;
      }
      assert(task);
      task->workOnIt();
      task->refDec();
    }
  }

  TaskSys::~TaskSys()
  {
    running = false;
    __memory_barrier();
    tasksAvailable.broadcast();
    for (int i = 0; i < threads.size(); ++i) {
      embree::join(threads[i]);
    }
  }

  void *TaskSys::threadStub(void *)
  {
    TaskSys::global.threadFunction();
    return NULL;
  }

  void TaskSys::init(size_t numThreads)
  {
    if (initialized)
      throw std::runtime_error("#osp: task system initialized twice!");
    initialized = true;
    running = true;

    if (numThreads != 0) {
#if defined(__MIC__)
      numThreads = std::min(numThreads,(size_t)embree::getNumberOfLogicalThreads()-4);
#else
      numThreads = std::min(numThreads,(size_t)embree::getNumberOfLogicalThreads());
#endif
    }
    PRINT(numThreads);

    /* generate all threads */
    for (size_t t=1; t<numThreads; t++) {
      // embree::createThread((embree::thread_func)TaskSys::threadStub,NULL,4*1024*1024,(t+1)%numThreads);
#if 0
      // embree will not assign affinity
      threads.push_back(embree::createThread((embree::thread_func)TaskSys::threadStub,(void*)-1,4*1024*1024,-1));
#else
      // embree will assign affinity in this case:
      threads.push_back(embree::createThread((embree::thread_func)TaskSys::threadStub,(void*)-1,4*1024*1024,t));
      embree::createThread((embree::thread_func)TaskSys::threadStub,(void*)t,4*1024*1024,t);
#endif
    }
    numActiveThreads = numThreads-1;

#if defined(__MIC__)
#else    
  //  embree::setAffinity(0);
#endif

    cout << "#osp: task system initialized" << endl;
  }

  /* Signature of ispc-generated 'task' functions */
  typedef void (*TaskFuncType)(void* data, int threadIndex, int threadCount, 
                               int taskIndex, int taskCount);

  struct ISPCTask : public Task {
    void        *data;
    TaskFuncType func;

    ISPCTask() : Task("ISPC Task"), data(NULL), func(NULL) {}

    virtual void run(size_t jobID) 
    {
      func(data,-1,-1,jobID,numJobsInTask);
    }
    
    virtual ~ISPCTask() 
    {
      _mm_free(data);
    }
  };

  __dllexport void* ISPCAlloc(void** taskPtr, int64 size, int32 alignment) 
  {
    if (*taskPtr == NULL) {
      ISPCTask *task = new ISPCTask;
      task->refInc();
      *taskPtr = task;
    }
    return (char*)_mm_malloc(size,alignment);
  }

  __dllexport void ISPCLaunch(void** taskPtr, void* func, void* data, int count) 
  {
    ISPCTask *task = (ISPCTask *)*taskPtr;
    task->func     = (TaskFuncType)func;
    task->data     = data;
    task->schedule(count);
  }
  
  __dllexport void ISPCSync(void* _task) 
  {
    ISPCTask *task = (ISPCTask *)_task;
    task->wait();
    task->refDec();
  }

}
