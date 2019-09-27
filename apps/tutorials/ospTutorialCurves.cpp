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

#include "GLFWOSPRayWindow.h"

#include "ospray_testing.h"

#include "tutorial_util.h"

#include <imgui.h>

using namespace ospcommon;

static std::string renderer_type = "pathtracer";

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  // create the world
  OSPWorld world = ospNewWorld();

  std::vector<OSPInstance> instanceHandles;

  // generate curves
  OSPTestingGeometry curves =
      ospTestingNewGeometry("curves", renderer_type.c_str());
  instanceHandles.push_back(curves.instance);
  ospRelease(curves.model);
  ospRelease(curves.geometry);
  ospRelease(curves.group);

  OSPInstance plane = createGroundPlane(renderer_type, 2.5f);
  instanceHandles.push_back(plane);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_INSTANCE, instanceHandles.data());

  ospSetObject(world, "instance", geomInstances);
  ospRelease(geomInstances);
  ospRelease(plane);
  ospRelease(curves.instance);

  OSPData lightsData = ospTestingNewLights("ambient_and_directional");
  ospSetObject(world, "light", lightsData);
  ospRelease(lightsData);

  ospCommit(world);

  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-2.f), vec3f(2.f)), world, renderer));

  // ImGui

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool update = false;

    if (update) {
      glfwOSPRayWindow->addObjectToCommit(curves.geometry);
      glfwOSPRayWindow->addObjectToCommit(curves.model);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
