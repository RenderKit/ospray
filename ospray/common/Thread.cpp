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

/*! \file ospray/common/Thread.h \brief Abstraction for a Thread object */

#include "Thread.h"
// embree
# include "ospcommon/thread.h"

namespace ospray {

  void ospray_Thread_runThread(void *arg)
  {
    Thread *t = (Thread *)arg;
    if (t->desiredThreadID >= 0) {
      printf("pinning to thread %i\n",t->desiredThreadID);
      ospcommon::setAffinity(t->desiredThreadID);
    }

    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    t->run();
  }

  void Thread::join() 
  { 
    ospcommon::join(tid); 
  }

  /*!  start thread execution */
  void Thread::start(int threadID)
  {
    desiredThreadID = threadID;
    this->tid = ospcommon::createThread(&ospray_Thread_runThread,this);
  }

}

