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

#include "TaskSys.h"
// ospray
#include "../../platform.h"
// stl
#include <thread>

namespace ospcommon {
  namespace tasking {
    namespace detail {

      // TaskSys definitions //////////////////////////////////////////////////

      static std::unique_ptr<enki::TaskScheduler> g_ts;

      // Interface definitions ////////////////////////////////////////////////

      void initTaskSystemInternal(int nThreads)
      {
        g_ts = std::unique_ptr<enki::TaskScheduler>(new enki::TaskScheduler());
        if (nThreads < 1)
          nThreads = enki::GetNumHardwareThreads();
        g_ts->Initialize(nThreads);
      }

      int numThreadsTaskSystemInternal()
      {
        return g_ts->GetNumTaskThreads();
      }

      void scheduleTaskInternal(Task *task)
      {
        if (g_ts.get() == nullptr)
          initTaskSystemInternal(-1);

        g_ts->AddTaskSetToPipe(task);
      }

      void waitInternal(Task *task)
      {
        g_ts->WaitforTask(task);
      }

    }  // namespace detail
  }    // namespace tasking
}  // namespace ospcommon
