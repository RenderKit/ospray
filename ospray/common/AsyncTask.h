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

#include "Managed.h"

#include <atomic>
#include <functional>
#include <future>
// ospcommon
#include "ospcommon/tasking/async.h"

namespace ospray {

  struct BaseTask : public ManagedObject
  {
    virtual ~BaseTask() override = default;

    virtual bool isFinished() const = 0;
    virtual bool isValid() const    = 0;
    virtual void wait() const = 0;
  };

  template <typename T>
  struct AsyncTask : public BaseTask
  {
    AsyncTask(std::function<T()> fcn);
    ~AsyncTask() override;

    bool isFinished() const override;
    bool isValid() const override;
    void wait() const override;

    T get();

   private:
    std::function<T()> stashedTask;
    std::future<T> runningTask;
    std::atomic<bool> jobFinished;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename T>
  AsyncTask<T>::AsyncTask(std::function<T()> fcn)
  {
    stashedTask    = fcn;
    jobFinished    = false;
    auto *thisTask = this;

    runningTask = ospcommon::tasking::async([=]() {
      T retval = thisTask->stashedTask();

      thisTask->jobFinished = true;
      return retval;
    });
  }

  template <typename T>
  inline AsyncTask<T>::~AsyncTask()
  {
    if (isValid())
      runningTask.wait();
  }

  template <typename T>
  inline bool AsyncTask<T>::isFinished() const
  {
    return jobFinished;
  }

  template <typename T>
  inline bool AsyncTask<T>::isValid() const
  {
    return runningTask.valid();
  }

  template <typename T>
  inline void AsyncTask<T>::wait() const
  {
    return runningTask.wait();
  }

  template <typename T>
  inline T AsyncTask<T>::get()
  {
    return runningTask.get();
  }

}  // namespace ospray
