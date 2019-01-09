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

#include "common.h"

namespace ospcommon
{
  /*! type for thread */
  typedef struct opaque_thread_t* thread_t;

  /*! signature of thread start function */
  typedef void (*thread_func)(void*);

  /*! creates a hardware thread running on specific logical thread */
  OSPCOMMON_INTERFACE thread_t createThread(thread_func f, void* arg, size_t
      stack_size = 0, ssize_t threadID = -1);

  /*! set affinity of the calling thread */
  OSPCOMMON_INTERFACE void setAffinity(ssize_t affinity);

  /*! the thread calling this function gets yielded */
  OSPCOMMON_INTERFACE void yield();

  /*! waits until the given thread has terminated */
  OSPCOMMON_INTERFACE void join(thread_t tid);

  /*! destroy handle of a thread */
  OSPCOMMON_INTERFACE void destroyThread(thread_t tid);

  struct OSPCOMMON_INTERFACE Thread
  {
    Thread() = default;
    virtual ~Thread() = default;

    /*! the actual run function that the newly started thread will execute */
    virtual void run() = 0;

    /*! start thread execution */
    void start(int threadID=-1);
    /*! join the specific thread, waiting for its termination */
    void join();

    friend void ospray_Thread_runThread(void *arg);
  private:
    /*! after starting, the thread will try to pin itself to this
        thread ID (if > -1). note that changing the value _after_ the
        thread got started will not have any effect */
    int desiredThreadID {-1};
    //! the thread ID reported by embree's createThread
    ospcommon::thread_t tid;
  };

}
