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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

#include "TypeTraits.h"

#include "tasking/schedule.h"
#include "tasking/tasking_system_handle.h"

namespace ospcommon {

  /*! This calls a given function in a continuous loop on a background
      thread owned by AsyncLoop. While it is running, the function it was
      constructed with is called over and over in a loop. When stopped, the
      thread is put to sleep until it is started again.

      An AsyncLoop has to be explicitly started, it is not automatically
      started on construction.
   */
  class AsyncLoop
  {
  public:

    enum LaunchMethod { AUTO = 0, THREAD = 1, TASK = 2 };

    template <typename LOOP_BODY_FCN>
    AsyncLoop(LOOP_BODY_FCN &&fcn, LaunchMethod m = AUTO);

    ~AsyncLoop();

    void start();
    void stop();

  private:

    // Struct shared with the background thread to avoid dangling ptrs or
    // tricky synchronization when destroying the AsyncLoop and scheduling
    // threads with TBB, since we don't have a join point to sync with
    // the running thread 
    struct AsyncLoopData {
      std::atomic<bool> threadShouldBeAlive {true};
      std::atomic<bool> shouldBeRunning     {false};
      std::atomic<bool> insideLoopBody      {false};

      std::condition_variable runningCond;
      std::mutex              runningMutex;
    };

    std::shared_ptr<AsyncLoopData> loop;
    std::thread                    backgroundThread;
  };

  // Inlined members //////////////////////////////////////////////////////////

  template <typename LOOP_BODY_FCN>
  inline AsyncLoop::AsyncLoop(LOOP_BODY_FCN &&fcn, AsyncLoop::LaunchMethod m)
    : loop(nullptr)
  {
    static_assert(traits::has_operator_method<LOOP_BODY_FCN>::value,
                  "ospcommon::AsyncLoop() requires the implementation of "
                  "method 'void LOOP_BODY_FCN::operator()' in order to "
                  "construct the loop instance.");

    std::shared_ptr<AsyncLoopData> l = std::make_shared<AsyncLoopData>();
    loop = l;

    auto mainLoop = [l,fcn]() {
      while (l->threadShouldBeAlive) {
        if (!l->threadShouldBeAlive)
          return;

        if (l->shouldBeRunning) {
          l->insideLoopBody = true;
          fcn();
          l->insideLoopBody = false;
        } else {
          std::unique_lock<std::mutex> lock(l->runningMutex);
          l->runningCond.wait(lock, [&] {
            return l->shouldBeRunning.load() || !l->threadShouldBeAlive.load();
          });
        }
      }
    };

    if (m == AUTO)
      m = tasking::numTaskingThreads() > 4 ? TASK : THREAD;

    if (m == THREAD)
      backgroundThread = std::thread(mainLoop);
    else // m == TASK
      tasking::schedule(mainLoop);
  }

  inline AsyncLoop::~AsyncLoop()
  {
    // Note that the mutex here is still required even though these vars
    // are atomic, because we need to sync with the condition variable waiting
    // state on the async thread. Otherwise we might signal and the thread
    // will miss it, since it wasn't watching.
    {
      std::unique_lock<std::mutex> lock(loop->runningMutex);
      loop->threadShouldBeAlive = false;
      loop->shouldBeRunning = false;
    }
    loop->runningCond.notify_one();

    if (backgroundThread.joinable()) {
      backgroundThread.join();
    }
  }

  inline void AsyncLoop::start()
  {
    if (!loop->shouldBeRunning) {
      // Note that the mutex here is still required even though these vars
      // are atomic, because we need to sync with the condition variable waiting
      // state on the async thread. Otherwise we might signal and the thread
      // will miss it, since it wasn't watching.
      {
        std::unique_lock<std::mutex> lock(loop->runningMutex);
        loop->shouldBeRunning = true;
      }
      loop->runningCond.notify_one();
    }
  }

  inline void AsyncLoop::stop()
  {
    if (loop->shouldBeRunning) {
      loop->shouldBeRunning = false;
      while (loop->insideLoopBody.load()) {
        std::this_thread::yield();
      }
    }
  }

} // ::ospcommon
