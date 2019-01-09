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

    std::atomic<bool> threadShouldBeAlive {true};
    std::atomic<bool> loopShouldBeRunning {false};
    std::atomic<bool> insideLoopBody      {false};

    std::thread             backgroundThread;
    std::condition_variable loopRunningCond;
    std::mutex              loopRunningMutex;
  };

  // Inlined members //////////////////////////////////////////////////////////

  template <typename LOOP_BODY_FCN>
  inline AsyncLoop::AsyncLoop(LOOP_BODY_FCN &&fcn, AsyncLoop::LaunchMethod m)
  {
    static_assert(traits::has_operator_method<LOOP_BODY_FCN>::value,
                  "ospcommon::AsyncLoop() requires the implementation of "
                  "method 'void LOOP_BODY_FCN::operator()' in order to "
                  "construct the loop instance.");

    auto mainLoop = [&,fcn]() {
      while (threadShouldBeAlive) {
        std::unique_lock<std::mutex> lock(loopRunningMutex);
        loopRunningCond.wait(lock, [&] { return loopShouldBeRunning.load(); });

        if (!threadShouldBeAlive)
          return;

        insideLoopBody = true;
        if(loopShouldBeRunning) fcn();
        insideLoopBody = false;
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
    threadShouldBeAlive = false;
    start();
    if (backgroundThread.joinable()) {
      backgroundThread.join();
    }
  }

  inline void AsyncLoop::start()
  {
    if (!loopShouldBeRunning) {
      loopShouldBeRunning = true;
      loopRunningCond.notify_one();
    }
  }

  inline void AsyncLoop::stop()
  {
    if (loopShouldBeRunning) {
      loopShouldBeRunning = false;
      std::unique_lock<std::mutex> lock(loopRunningMutex);
      loopRunningCond.wait(lock, [&] { return !insideLoopBody.load(); });
    }
  }

} // ::ospcommon
