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

// ospray
#include "Device.h"
#include "common/OSPCommon.h"
#include "objectFactory.h"
#include "ospray/version.h"
// ospcommon
#include "ospcommon/os/library.h"
#include "ospcommon/utility/getEnvVar.h"
#include "ospcommon/tasking/tasking_system_init.h"

#include <map>

namespace ospray {
  namespace api {

    // Helper functions ///////////////////////////////////////////////////////

    template <typename OSTREAM_T>
    static inline void installStatusMsgFunc(Device& device, OSTREAM_T &stream)
    {
      device.msg_fcn = [&](const char *msg){ stream << msg; };
    }

    template <typename OSTREAM_T>
    static inline void installErrorMsgFunc(Device& device, OSTREAM_T &stream)
    {
      device.error_fcn = [&](OSPError e, const char *msg) {
        stream << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
      };
    }

    // Device definitions /////////////////////////////////////////////////////

    memory::IntrusivePtr<Device> Device::current;
    uint32_t Device::logLevel = 0;

    Device *Device::createDevice(const char *type)
    {
      // NOTE(jda) - If a user is manually creating the device (i.e. not using
      //             ospInit() to do it), then we need to check if there's a
      //             valid library for core ospray in our main symbol lookup
      //             table.
      auto &repo = *LibraryRepository::getInstance();
      if (!repo.libraryExists("ospray")) {
        repo.addDefaultLibrary();
        // also load the local device, otherwise ospNewDevice("default") fails
        repo.add("ospray_module_ispc");
      }

      return objectFactory<Device, OSP_DEVICE>(type);
    }

    void Device::commit()
    {
      auto OSPRAY_DEBUG = utility::getEnvVar<int>("OSPRAY_DEBUG");
      debugMode = OSPRAY_DEBUG.value_or(getParam<bool>("debug", 0));

      auto OSPRAY_TRACE_API = utility::getEnvVar<int>("OSPRAY_TRACE_API");
      bool traceAPI = OSPRAY_TRACE_API.value_or(getParam<bool>("traceApi", 0));

      if (traceAPI && !apiTraceEnabled) {
        auto streamPtr =
          std::make_shared<std::ofstream>("ospray_api_trace.txt");

        trace_fcn = [=](const char *message) {
          auto &stream = *streamPtr;
          stream << message << std::endl;
        };
      } else if (!traceAPI) {
        trace_fcn = [](const char *) {};
      }

      apiTraceEnabled = traceAPI;

      auto OSPRAY_LOG_LEVEL = utility::getEnvVar<int>("OSPRAY_LOG_LEVEL");
      logLevel = OSPRAY_LOG_LEVEL.value_or(getParam<int>("logLevel", 0));

      auto OSPRAY_THREADS = utility::getEnvVar<int>("OSPRAY_THREADS");
      numThreads = OSPRAY_THREADS.value_or(getParam<int>("numThreads", -1));

      auto OSPRAY_LOG_OUTPUT =
          utility::getEnvVar<std::string>("OSPRAY_LOG_OUTPUT");

      auto dst = OSPRAY_LOG_OUTPUT.value_or(
        getParam<std::string>("logOutput", "")
      );

      if (dst == "cout")
        installStatusMsgFunc(*this, std::cout);
      else if (dst == "cerr")
        installStatusMsgFunc(*this, std::cerr);
      else if (dst == "none")
        msg_fcn = [](const char*){};

      auto OSPRAY_ERROR_OUTPUT =
          utility::getEnvVar<std::string>("OSPRAY_ERROR_OUTPUT");

      dst = OSPRAY_ERROR_OUTPUT.value_or(
        getParam<std::string>("errorOutput", "")
      );

      if (dst == "cout")
        installErrorMsgFunc(*this, std::cout);
      else if (dst == "cerr")
        installErrorMsgFunc(*this, std::cerr);
      else if (dst == "none")
        error_fcn = [](OSPError, const char*){};

      if (debugMode) {
        logLevel   = 2;
        numThreads = 1;
        installStatusMsgFunc(*this, std::cout);
        installErrorMsgFunc(*this, std::cerr);
      }

      threadAffinity = AUTO_DETECT;

      auto OSPRAY_SET_AFFINITY = utility::getEnvVar<int>("OSPRAY_SET_AFFINITY");
      if (OSPRAY_SET_AFFINITY)
        threadAffinity = OSPRAY_SET_AFFINITY.value();

      threadAffinity = getParam<int>("setAffinity", threadAffinity);

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
        embreeConfig << " verbose=2";

      if (device.threadAffinity == api::Device::AFFINITIZE)
        embreeConfig << ",set_affinity=1";
      else if (device.threadAffinity == api::Device::DEAFFINITIZE)
        embreeConfig << ",set_affinity=0";

      return embreeConfig.str();
    }

    int64_t Device::getProperty(const OSPDeviceProperty prop)
    {
      /* documented properties */
      switch (prop)
      {
      case OSP_DEVICE_VERSION           :
          return 10000*OSPRAY_VERSION_MAJOR + 100*OSPRAY_VERSION_MINOR + OSPRAY_VERSION_PATCH;
      case OSP_DEVICE_VERSION_MAJOR     : return OSPRAY_VERSION_MAJOR;
      case OSP_DEVICE_VERSION_MINOR     : return OSPRAY_VERSION_MINOR;
      case OSP_DEVICE_VERSION_PATCH     : return OSPRAY_VERSION_PATCH;
      case OSP_DEVICE_SO_VERSION        : return OSPRAY_SOVERSION;
      default: handleError(OSP_INVALID_ARGUMENT, "unknown readable property");
      return 0;
      }
    }
  } // ::ospray::api

  OSPTYPEFOR_DEFINITION(api::Device *);

} // ::ospray
