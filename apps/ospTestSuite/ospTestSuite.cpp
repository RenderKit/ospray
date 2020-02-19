// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

extern OSPRayEnvironment *ospEnv;
OSPRayEnvironment *ospEnv;

int main(int argc, char **argv)
{
  ospInit(&argc, (const char **)argv);

  {
    cpp::Device device = cpp::Device::current();

    device.setErrorFunc([](OSPError error, const char *errorDetails) {
      std::cerr << "OSPRay error: " << errorDetails << std::endl;
      std::exit(EXIT_FAILURE);
    });

    device.setStatusFunc([](const char *msg) { std::cout << msg; });

    device.setParam("warnAsError", true);
    device.setParam("logLevel", OSP_LOG_WARNING);

    device.commit();
  }

  ::testing::InitGoogleTest(&argc, argv);
  ospEnv = new OSPRayEnvironment(argc, argv);
  AddGlobalTestEnvironment(ospEnv);
  auto result = RUN_ALL_TESTS();

  ospShutdown();

  return result;
}
