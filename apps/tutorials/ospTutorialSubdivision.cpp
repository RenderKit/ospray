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

#include <iterator>
#include <memory>
#include <random>
#include "GLFWOSPRayWindow.h"

#include "ospcommon/library.h"
#include "ospray_testing.h"

#include "tutorial_util.h"

#include <imgui.h>

using namespace ospcommon;

static std::string renderer_type = "scivis";

int main(int argc, const char **argv)
{
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // create the world which will contain all of our geometries
  OSPWorld world = ospNewWorld();

  std::vector<OSPInstance> instanceHandles;

  // add in subdivision geometry
  OSPTestingGeometry subdivisionGeometry =
      ospTestingNewGeometry("subdivision_cube", renderer_type.c_str());
  instanceHandles.push_back(subdivisionGeometry.instance);
  ospRelease(subdivisionGeometry.geometry);
  ospRelease(subdivisionGeometry.model);

  // add in a ground plane geometry
  OSPInstance plane = createGroundPlane(renderer_type);
  instanceHandles.push_back(plane);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());

  ospSetData(world, "instance", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_and_directional");
  ospSetData(renderer, "light", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    static int tessellationLevel = 5;
    if (ImGui::SliderInt("tessellation level", &tessellationLevel, 1, 10)) {
      ospSetFloat(subdivisionGeometry.geometry, "level", tessellationLevel);
      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.geometry);
      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }

    static float vertexCreaseWeight = 2.f;
    if (ImGui::SliderFloat(
            "vertex crease weights", &vertexCreaseWeight, 0.f, 5.f)) {
      // vertex crease indices already set on cube geometry

      // vertex crease weights; use a constant weight for each vertex
      std::vector<float> vertexCreaseWeights(8);
      std::fill(vertexCreaseWeights.begin(),
                vertexCreaseWeights.end(),
                vertexCreaseWeight);

      OSPData vertexCreaseWeightsData = ospNewData(
          vertexCreaseWeights.size(), OSP_FLOAT, vertexCreaseWeights.data());

      ospSetData(subdivisionGeometry.geometry,
                 "vertexCrease.weight",
                 vertexCreaseWeightsData);
      ospRelease(vertexCreaseWeightsData);

      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.geometry);
      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }

    static float edgeCreaseWeight = 2.f;
    if (ImGui::SliderFloat(
            "edge crease weights", &edgeCreaseWeight, 0.f, 5.f)) {
      // edge crease indices already set on cube geometry

      // edge crease weights; use a constant weight for each edge
      std::vector<float> edgeCreaseWeights(12);
      std::fill(
          edgeCreaseWeights.begin(), edgeCreaseWeights.end(), edgeCreaseWeight);

      OSPData edgeCreaseWeightsData = ospNewData(
          edgeCreaseWeights.size(), OSP_FLOAT, edgeCreaseWeights.data());

      ospSetData(subdivisionGeometry.geometry,
                 "edgeCrease.weight",
                 edgeCreaseWeightsData);
      ospRelease(edgeCreaseWeightsData);

      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.geometry);
      glfwOSPRayWindow->addObjectToCommit(subdivisionGeometry.group);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
