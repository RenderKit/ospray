// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GLFWOSPRayWindow.h"
#include "example_common.h"

using namespace ospray;
using ospcommon::make_unique;

int main(int argc, const char *argv[])
{
  initializeOSPRay(argc, argv);

  auto glfwOSPRayWindow = make_unique<GLFWOSPRayWindow>(vec2i(1024, 768));
  glfwOSPRayWindow->mainLoop();
  glfwOSPRayWindow.reset();

  ospShutdown();

  return 0;
}
