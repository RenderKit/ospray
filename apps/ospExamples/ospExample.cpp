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

#include "example_common.h"
#include "GLFWOSPRayWindow.h"

using namespace ospray;

static std::string rendererType = "pathtracer";

int main(int argc, const char *argv[])
{
  initializeOSPRay(argc, argv);

  {
    // auto builder = testing::newBuilder("gravity_spheres_volume");
    // auto builder = testing::newBuilder("random_spheres");
    // auto builder = testing::newBuilder("cornell_box");
    // auto builder = testing::newBuilder("boxes");
    // auto builder = testing::newBuilder("curves");
    // auto builder = testing::newBuilder("streamlines");
    // auto builder = testing::newBuilder("cylinders");
    // auto builder = testing::newBuilder("subdivision_cube");
    auto builder = testing::newBuilder("unstructured_volume");
    testing::setParam(builder, "rendererType", rendererType);
    testing::commit(builder);

    auto world = testing::buildWorld(builder);
    testing::release(builder);

    world.commit();

    cpp::Renderer renderer(rendererType);
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
