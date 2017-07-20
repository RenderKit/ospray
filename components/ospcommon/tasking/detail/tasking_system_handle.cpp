// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "../tasking_system_handle.h"

// tasking system internals
#if defined(OSPRAY_TASKING_TBB)
# include <tbb/task_scheduler_init.h>
#elif defined(OSPRAY_TASKING_CILK)
# include <cilk/cilk_api.h>
#elif defined(OSPRAY_TASKING_OMP)
# include <omp.h>
#elif defined(OSPRAY_TASKING_INTERNAL)
# include "TaskSys.h"
#endif

#include <thread>

#include "../../intrinsics.h"
#include "../../common.h"

namespace ospcommon {
  namespace tasking {

    struct tasking_system_handle
    {
      tasking_system_handle(int numThreads) :
        numThreads(numThreads)
#if defined(OSPRAY_TASKING_TBB)
        , tbb_init(numThreads)
#endif
      {
#if defined(OSPRAY_TASKING_CILK)
        __cilkrts_set_param("nworkers", std::to_string(numThreads).c_str());
#elif defined(OSPRAY_TASKING_OMP)
         if (numThreads > 0) omp_set_num_threads(numThreads);
#elif defined(OSPRAY_TASKING_INTERNAL)
         detail::initTaskSystemInternal(numThreads < 0 ? -1 : numThreads);
#endif
      }

      int numThreads {-1};
#if defined(OSPRAY_TASKING_TBB)
      tbb::task_scheduler_init tbb_init;
#endif
    };

    static std::unique_ptr<tasking_system_handle> g_tasking_handle;

    void initTaskingSystem(int numThreads)
    {
      _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
      _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

#if defined(OSPRAY_TASKING_TBB)
      if (!g_tasking_handle.get())
        g_tasking_handle = make_unique<tasking_system_handle>(numThreads);
      else {
        g_tasking_handle->tbb_init.terminate();
        g_tasking_handle->tbb_init.initialize(numThreads);
      }
#else
      g_tasking_handle = make_unique<tasking_system_handle>(numThreads);
#endif
    }

    int numTaskingThreads()
    {
      if (!g_tasking_handle.get())
        return 0;
      else if (g_tasking_handle->numThreads < 0)
        return std::thread::hardware_concurrency();
      else
        return g_tasking_handle->numThreads;
    }

    void deAffinitizeCores()
    {
#ifdef __linux__
      cpu_set_t validCores;
      CPU_ZERO(&validCores);
      for (int i=0;i<CPU_SETSIZE;i++)
        CPU_SET(i,&validCores);
      int rc = sched_setaffinity(getpid(),sizeof(validCores),&validCores);
      if (rc != 0) throw std::runtime_error("Error setting thread affinity!");
#endif
    }

  } // ::ospcommon::tasking
} // ::ospcommon
