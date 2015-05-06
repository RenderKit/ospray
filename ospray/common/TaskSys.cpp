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
// stl - ugh.
#include <deque>
// embree
#include <common/sys/sysinfo.h>

namespace ospray {
  using std::cout;
  using std::endl;

  struct TaskSys {
    bool initialized;

    void init(size_t maxNumRenderTasks);
    static TaskSys global;
    static void *threadStub(void *);
    inline Task *getNextActiveTask();

    //! queue of tasks that have ALREADY been acitvated, and that are ready to run
    Task *volatile __aligned(64) activeListFirst;
    Task *volatile __aligned(64) activeListLast;

    embree::MutexSys     __aligned(64) mutex;
    embree::ConditionSys __aligned(64) tasksAvailable;

    void threadFunction();

    TaskSys()
      : activeListFirst(NULL), activeListLast(NULL), initialized(0)
    {}
  };
  
  TaskSys __aligned(64) TaskSys::global;

    //! one of our depdencies tells us that he's finished
  void Task::oneDependencyGotCompleted(Task *which)
  {
#if TASKSYS_DEPENDENCIES
    mutex.lock();
    if (--numMissingDependencies == 0) {
      allDependenciesFulfilledCond.broadcast();
      activate();
    }
    mutex.unlock();
#endif
  }

  inline void Task::workOnIt() 
  {
    // work on dependencies, until they are done
    if (numMissingDependencies > 0) {
#if TASKSYS_DEPENDENCIES
      for (size_t i=0;i<dependency.size();i++)
        dependency[i]->workOnIt();
      mutex.lock();
      while (numMissingDependencies)
        allDependenciesFulfilledCond.wait(mutex);
      mutex.unlock();
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
        
        mutex.lock();
	// __memory_barrier();
        status = Task::COMPLETED;
	// __memory_barrier();
        allJobsCompletedCond.broadcast();
        mutex.unlock();
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
      mutex.lock();
      while (1) {
	// __memory_barrier();
        const Status status = this->status;
	// __memory_barrier();
        if (status == Task::COMPLETED)
          break;
        allJobsCompletedCond.wait(mutex);
      }
      mutex.unlock();
    }
  }


  void Task::scheduleAndWait(size_t numJobs)
  {
    schedule(numJobs);
    wait();
  }
  //   refInc();
  //   numJobsInTask = numJobs;
  //   status = Task::SCHEDULED;
  //   if (numMissingDependencies == 0)
  //     activate();

  //   this->workOnIt();

  //   if (status != COMPLETED) {
  //     mutex.lock();
  //     while (1) {
  //       // __memory_barrier();
  //       const Status status = this->status;
  //       // __memory_barrier();
  //       if (status == Task::COMPLETED)
  //         break;
  //       allJobsCompletedCond.wait(mutex);
  //     }
  //     mutex.unlock();
  //   }
  // }
  


  inline Task *TaskSys::getNextActiveTask()
  {
    mutex.lock();
    while (1) {
      while (activeListFirst == NULL)
        tasksAvailable.wait(mutex);
      Task *const front = activeListFirst;
      if (front->numJobsStarted >= front->numJobsInTask) {
        if (activeListFirst == activeListLast) {
          activeListFirst = activeListLast = NULL;
        } else {
          activeListFirst = activeListFirst->next;
        }
        // mutex.unlock();
        front->refDec();
        // mutex.lock();
        continue;
      }
      front->refInc(); // becasue the thread calling us now owns it
      mutex.unlock();
      assert(front);
      return front;
    }
  }
    
  void Task::initTaskSystem(const size_t maxNumRenderTasks)
  {
    TaskSys::global.init(maxNumRenderTasks);
  }

  void Task::schedule(size_t numJobs)
  {
    refInc();
    numJobsInTask = numJobs;
    status = Task::SCHEDULED;
    if (numMissingDependencies == 0)
      activate();
  }

  inline void Task::activate()
  {
    if (!TaskSys::global.initialized)
      throw std::runtime_error("TASK SYSTEM NOT YET INITIALIZED");
    TaskSys::global.mutex.lock();
    bool wasEmpty = TaskSys::global.activeListFirst == NULL;
    if (wasEmpty) {
      TaskSys::global.activeListFirst = TaskSys::global.activeListLast = this;
      this->prev = this->next = NULL;
      TaskSys::global.tasksAvailable.broadcast();
    } else {
      this->next = NULL;
      this->prev = TaskSys::global.activeListLast;
      this->prev->next = this;
      TaskSys::global.activeListLast = this;      
    }
    status = Task::ACTIVE;
    TaskSys::global.mutex.unlock();
  }


  void TaskSys::threadFunction()
  {
    while (1) {
      Task *task = getNextActiveTask();
      assert(task);
      task->workOnIt();
      task->refDec();
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

    if (numThreads != 0) {
#if defined(__MIC__)
      numThreads = std::min(numThreads,(size_t)embree::getNumberOfLogicalThreads()-4);
#else
      numThreads = std::min(numThreads,(size_t)embree::getNumberOfLogicalThreads());
#endif
    }

    /* generate all threads */
    for (size_t t=1; t<numThreads; t++) {
      // embree::createThread((embree::thread_func)TaskSys::threadStub,NULL,4*1024*1024,(t+1)%numThreads);
#if 1
      // embree will not assign affinity
      embree::createThread((embree::thread_func)TaskSys::threadStub,(void*)-1,4*1024*1024,-1);
#else
      // embree will assign affinity in this case:
      embree::createThread((embree::thread_func)TaskSys::threadStub,(void*)t,4*1024*1024,t);
#endif
    }

#if defined(__MIC__)
#else    
    embree::setAffinity(0);
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
    // ISPCTask* ispcTask = new ISPCTask((TaskScheduler::Event*)(*taskPtr),(TaskFuncType)func,data,count);
    // TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &ispcTask->task);
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
