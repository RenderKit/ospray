﻿// ======================================================================== //
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

// ospray
#include "Device.h"
#include "common/OSPCommon.h"
#include "common/Util.h"
#include "ospcommon/sysinfo.h"
// tasking system internals
#include "ospcommon/tasking/tasking_system_handle.h"
// embree
#include "embree2/rtcore.h"

#include <map>

namespace ospray {

  namespace api {

    Ref<Device> Device::current = nullptr;

    RTCDevice Device::embreeDevice = nullptr;
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

      if ((cpuFeatures & ospcommon::CPU_FEATURE_SSE41) == 0) {
        handleError(OSP_UNSUPPORTED_CPU,
                    "OSPRay only runs on CPUs that support at least SSE4.1");
        return;
      }

      auto OSPRAY_DEBUG = getEnvVar<int>("OSPRAY_DEBUG");
      debugMode = OSPRAY_DEBUG.first ? OSPRAY_DEBUG.second :
                                       getParam1i("debug", 0);

      auto OSPRAY_TRACE_API = getEnvVar<int>("OSPRAY_TRACE_API");
      bool traceAPI = OSPRAY_TRACE_API.first ? OSPRAY_TRACE_API.second :
                                               getParam1i("traceApi", 0);
      if (traceAPI) {
        auto streamPtr =
          std::make_shared<std::ofstream>("ospray_api_trace.txt");

        trace_fcn = [=](const char *message) {
          auto &stream = *streamPtr;
          stream << message << std::endl;
        };
      }

      auto OSPRAY_LOG_LEVEL = getEnvVar<int>("OSPRAY_LOG_LEVEL");
      logLevel = OSPRAY_LOG_LEVEL.first ? OSPRAY_LOG_LEVEL.second :
                                          getParam1i("logLevel", 0);

      auto OSPRAY_THREADS = getEnvVar<int>("OSPRAY_THREADS");
      numThreads = OSPRAY_THREADS.first ? OSPRAY_THREADS.second :
                                          getParam1i("numThreads", -1);

      auto OSPRAY_LOG_OUTPUT = getEnvVar<std::string>("OSPRAY_LOG_OUTPUT");
      if (OSPRAY_LOG_OUTPUT.first) {
        auto &dst = OSPRAY_LOG_OUTPUT.second;
        if (dst == "cout")
          msg_fcn = [](const char *msg){ std::cout << msg; };
        else if (dst == "cerr")
          msg_fcn = [](const char *msg){ std::cerr << msg; };
      }

      if (debugMode) {
        logLevel   = 2;
        numThreads = 1;
      }

      auto OSPRAY_SET_AFFINITY = getEnvVar<int>("OSPRAY_SET_AFFINITY");
      if (OSPRAY_SET_AFFINITY.first) {
        threadAffinity = OSPRAY_SET_AFFINITY.second == 0 ? DEAFFINITIZE :
                                                           AFFINITIZE;
      }

      threadAffinity = getParam1i("setAffinity", threadAffinity);

      tasking::initTaskingSystem(numThreads);

      committed = true;
    }

    bool Device::isCommitted()
    {
      return committed;
    }

    bool deviceIsSet()
    {
      return Device::current.ptr != nullptr;
    }

    Device &currentDevice()
    {
      return *Device::current;
    }

    std::string generateEmbreeDeviceCfg(const Device &device)
    {
      std::stringstream embreeConfig;

      if (device.debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(device.numThreads > 0)
        embreeConfig << " threads=" << device.numThreads;

      if (device.threadAffinity == api::Device::AFFINITIZE)
        embreeConfig << ",set_affinity=1";
      else if (device.threadAffinity == api::Device::DEAFFINITIZE)
        embreeConfig << ",set_affinity=0";

      return embreeConfig.str();
    }

  } // ::ospray::api
} // ::ospray
