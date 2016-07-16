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

/*! \file ospray/common/Thread.h \brief Abstraction for a Thread object */

// ospray
#include "OSPCommon.h"
// ospcommon
#include "ospcommon/thread.h"

namespace ospray {
  
  struct Thread {
    Thread() : desiredThreadID(-1) {}

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
    int desiredThreadID;
    //! the thread ID reported by embree's createThread
    ospcommon::thread_t tid;
  };

}

