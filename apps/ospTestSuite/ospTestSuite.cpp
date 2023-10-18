// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

#ifdef SYCL_LANGUAGE_VERSION
#include "sycl/sycl.hpp"
#endif

OSPRayEnvironment *ospEnv;

int main(int argc, char **argv)
{
  {
    // Check to make sure these functions work without a currently set device
    std::cout << "Verify device API usage prior to ospInit()..." << std::endl;
    auto d = ospGetCurrentDevice();
    ospDeviceRetain(d);
    ospDeviceRelease(d);
    std::cout << "...done!" << std::endl;
  }

  ospInit(&argc, (const char **)argv);

  ::testing::InitGoogleTest(&argc, argv);
  ospEnv = new OSPRayEnvironment(argc, argv);
  AddGlobalTestEnvironment(ospEnv);

  {
    cpp::Device device = cpp::Device::current();

#ifdef SYCL_LANGUAGE_VERSION
    sycl::queue *appsSyclQueue = ospEnv->GetAppsSyclQueue();
    if (appsSyclQueue) {
      static auto appsSyclContext = appsSyclQueue->get_context();
      static auto appsSyclDevice = appsSyclQueue->get_device();
      device.setParam("syclContext", OSP_VOID_PTR, (void *)&appsSyclContext);
      device.setParam("syclDevice", OSP_VOID_PTR, (void *)&appsSyclDevice);
    }
#endif

    device.setErrorCallback(
        [](void *, OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          std::exit(EXIT_FAILURE);
        });

    device.setStatusCallback([](void *, const char *msg) { std::cout << msg; });

    // First commit with INFO log Level to output device info string
    device.setParam("warnAsError", true);
    device.setParam("logLevel", OSP_LOG_INFO);
    device.commit();

    // Then use WARNING log level for tests execution
    device.setParam("logLevel", OSP_LOG_WARNING);
    device.commit();
  }

  auto result = RUN_ALL_TESTS();

  ospShutdown();

  return result;
}
