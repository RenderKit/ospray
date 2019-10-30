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
#include "example_common.h"

using namespace ospray;

static std::string rendererType = "pathtracer";
static std::string builderType  = "perlin_noise_volumes";

void printHelp()
{
  std::cout <<
      R"description(
usage: ./ospExamples [-h | --help] [[-s | --scene] scene] [[r | --renderer] renderer_type]

scenes:

  boxes
  cornell_box
  curves
  cylinders
  empty
  gravity_spheres_volume
  perlin_noise_volumes
  random_spheres
  streamlines
  subdivision_cube
  unstructured_volume

  )description";
}

int main(int argc, const char *argv[])
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      printHelp();
      return 0;
    } else if (arg == "-r" || arg == "--renderer") {
      rendererType = argv[++i];
    } else if (arg == "-s" || arg == "--scene") {
      builderType = argv[++i];
    }
  }

  {
    auto builder = testing::newBuilder(builderType);
    testing::setParam(builder, "rendererType", rendererType);
    testing::commit(builder);

    auto world = testing::buildWorld(builder);
    testing::release(builder);

    world.commit();

    // create a GLFW OSPRay window: this object will create and manage the
    // OSPRay frame buffer and camera directly
    auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
        new GLFWOSPRayWindow(vec2i(1024, 768), world, rendererType));

    // start the GLFW main loop, which will continuously render
    glfwOSPRayWindow->mainLoop();
  }

  ospShutdown();

  return 0;
}
