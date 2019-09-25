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

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // put the instance in the world
  OSPWorld world = ospNewWorld();

  std::vector<OSPInstance> instanceHandles;

  // add in boxes geometry
  OSPTestingGeometry boxes =
      ospTestingNewGeometry("cornell_box", renderer_type.c_str());
  instanceHandles.push_back(boxes.instance);
  ospRelease(boxes.geometry);
  ospRelease(boxes.model);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_INSTANCE, instanceHandles.data());

  ospSetData(world, "instance", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  // Set up area light in the ceiling
  OSPLight light = ospNewLight("quad");
  ospSetVec3f(light, "color", 0.78f, 0.551f, 0.183f);
  ospSetFloat(light, "intensity", 47.f);
  ospSetVec3f(light, "position", -0.23f, 0.98f, -0.16f);
  ospSetVec3f(light, "edge1", 0.47f, 0.0f, 0.0f);
  ospSetVec3f(light, "edge2", 0.0f, 0.0f, 0.38f);

  ospCommit(light);
  OSPData lights = ospNewData(1, OSP_LIGHT, &light);
  ospCommit(lights);
  ospSetData(renderer, "light", lights);

  // finalize the renderer
  ospCommit(renderer);
  ospRelease(light);
  ospRelease(lights);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, (box3f &)boxes.bounds, world, renderer));

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
