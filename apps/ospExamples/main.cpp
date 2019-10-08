// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

// ospray_testing
#include "ospray_testing.h"
// stl
#include <iostream>

#include "GLFWOSPRayWindow.h"

using namespace ospray;

void initializeOSPRay(int argc, const char **argv, bool errorsFatal = true)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    throw std::runtime_error("OSPRay not initialized correctly!");

  OSPDevice device = ospGetCurrentDevice();
  if (!device)
    throw std::runtime_error("OSPRay device could not be fetched!");

  // set an error callback to catch any OSPRay errors and exit the application
  if (errorsFatal) {
    ospDeviceSetErrorFunc(device, [](OSPError error, const char *errorDetails) {
      std::cerr << "OSPRay error: " << errorDetails << std::endl;
      exit(error);
    });
  } else {
    ospDeviceSetErrorFunc(device, [](OSPError, const char *errorDetails) {
      std::cerr << "OSPRay error: " << errorDetails << std::endl;
    });
  }
}

int main(int argc, const char *argv[])
{
  initializeOSPRay(argc, argv);

  {
    auto builder = testing::newBuilder("gravity_spheres_volume");
    testing::commit(builder);

    auto world = testing::buildWorld(builder);
    testing::release(builder);

    cpp::Renderer renderer("scivis");
    renderer.commit();

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
        new GLFWOSPRayWindow(vec2i(1024, 768), world, renderer));

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();
  }

  ospShutdown();

  return 0;
}
