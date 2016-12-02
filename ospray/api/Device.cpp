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

// ospray
#include "Device.h"
#include "common/OSPCommon.h"
#include "common/Util.h"
#include "ospcommon/sysinfo.h"
// tasking system internals
#if defined(OSPRAY_TASKING_TBB)
# include <tbb/task_scheduler_init.h>
#elif defined(OSPRAY_TASKING_CILK)
# include <cilk/cilk_api.h>
#elif defined(OSPRAY_TASKING_OMP)
# include <omp.h>
#elif defined(OSPRAY_TASKING_INTERNAL)
# include "common/tasking/TaskSys.h"
#endif
// embree
#include "embree2/rtcore.h"

#include <map>

namespace ospray {

  void error_handler(const RTCError code, const char *str);

  namespace api {

    Ref<Device> Device::current = nullptr;

    uint32_t Device::logLevel = 0;

    Device::~Device()
    {
      if (embreeDevice) rtcDeleteDevice(embreeDevice);
    }

    Device *Device::createDevice(const char *type)
    {
      return createInstanceHelper<Device, OSP_DEVICE>(type);
    }

    void Device::commit()
    {
      int cpuFeatures = ospcommon::getCPUFeatures();
      if ((cpuFeatures & ospcommon::CPU_FEATURE_SSE41) == 0)
        throw std::runtime_error("Error. OSPRay only runs on CPUs that support"
                                 " at least SSE4.1.");


      auto OSPRAY_DEBUG = getEnvVar<int>("OSPRAY_DEBUG");
      debugMode = OSPRAY_DEBUG.first ? OSPRAY_DEBUG.second :
                                       getParam1i("debug", 0);

      auto OSPRAY_LOG_LEVEL = getEnvVar<int>("OSPRAY_LOG_LEVEL");
      logLevel = OSPRAY_LOG_LEVEL.first ? OSPRAY_LOG_LEVEL.second :
                                          getParam1i("logLevel", 0);

      auto OSPRAY_THREADS = getEnvVar<int>("OSPRAY_THREADS");
      numThreads = OSPRAY_THREADS.first ? OSPRAY_THREADS.second :
                                          getParam1i("numThreads", -1);

      if (debugMode) {
        logLevel   = 2;
        numThreads = 1;
      }

#if defined(OSPRAY_TASKING_TBB)
      static tbb::task_scheduler_init tbb_init(numThreads);
      UNUSED(tbb_init);
#elif defined(OSPRAY_TASKING_CILK)
      __cilkrts_set_param("nworkers", std::to_string(numThreads).c_str());
#elif defined(OSPRAY_TASKING_OMP)
      if (numThreads > 0) {
        omp_set_num_threads(numThreads);
      }
#elif defined(OSPRAY_TASKING_INTERNAL)
      try {
        ospray::Task::initTaskSystem(debugMode ? 0 : numThreads);
      } catch (const std::runtime_error &e) {
        std::cerr << "WARNING: " << e.what() << std::endl;
      }
#endif

      committed = true;
    }

    bool Device::isCommitted()
    {
      return committed;
    }

  } // ::ospray::api
} // ::ospray
