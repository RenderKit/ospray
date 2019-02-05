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

#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the "world" model which will contain all of our geometries / volumes
  OSPModel world = ospNewModel();

  // add in generated volume and transfer function
#if 0
  OSPTestingVolume test_data = ospTestingNewVolume("simple_structured_volume");
#else
  OSPTestingVolume test_data = ospTestingNewVolume("gravity_spheres_volume");
#endif

  OSPTransferFunction tfn =
      ospTestingNewTransferFunction(test_data.voxelRange, "jet");

  ospSetObject(test_data.volume, "transferFunction", tfn);
  ospCommit(test_data.volume);

  ospAddVolume(world, test_data.volume);
  ospRelease(test_data.volume);
  ospRelease(tfn);

  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("scivis");

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 768},
                           reinterpret_cast<box3f &>(test_data.bounds),
                           world,
                           renderer));

  glfwOSPRayWindow->registerImGuiCallback([=]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      ospCommit(renderer);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
