// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GLFWOSPRayWindow.h"
#include "example_common.h"

using namespace ospray;
using rkcommon::make_unique;

int main(int argc, const char *argv[])
{
  initializeOSPRay(argc, argv, false);

  const bool denoiserAvailable = ospLoadModule("denoiser") == OSP_NO_ERROR;

  bool denoiserGPUSupport = false;
  if (denoiserAvailable) {
    const void *sym =
        rkcommon::getSymbol("ospray_module_denoiser_gpu_supported");
    if (sym) {
      auto denoiserGPUSupported = (int (*)())sym;
      denoiserGPUSupport = denoiserGPUSupported();
    }
  }

  auto glfwOSPRayWindow = make_unique<GLFWOSPRayWindow>(
      vec2i(1024, 768), denoiserAvailable, denoiserGPUSupport);
  glfwOSPRayWindow->mainLoop();
  glfwOSPRayWindow.reset();

  ospShutdown();

  return 0;
}
