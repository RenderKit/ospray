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

static std::string renderer_type = "pathtracer";

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

  // add in spheres geometry
  OSPTestingGeometry spheres =
      ospTestingNewGeometry("spheres", renderer_type.c_str());
  instanceHandles.push_back(spheres.instance);
  ospRelease(spheres.geometry);
  ospRelease(spheres.model);

  // add in a ground plane geometry
  OSPInstance plane = createGroundPlane(renderer_type);
  instanceHandles.push_back(plane);

  OSPData geomInstances =
      ospNewData(instanceHandles.size(), OSP_OBJECT, instanceHandles.data());

  ospSetData(world, "instances", geomInstances);
  ospRelease(geomInstances);

  for (auto inst : instanceHandles)
    ospRelease(inst);

  // commit the world
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // Create a PixelOp pipeline to apply to the image
  OSPPixelOp toneMap = ospNewPixelOp("tonemapper");
  ospCommit(toneMap);

  // The colortweak pixel op will make the image more blue
  OSPPixelOp colorTweak = ospNewPixelOp("debug");
  ospSetVec3f(colorTweak, "addColor", 0.0, 0.0, 0.2);
  ospCommit(colorTweak);

  // UI for the tweaking the pixel pipeline
  std::vector<bool> enabledOps   = {true, true};
  std::vector<vec3f> debugColors = {vec3f(-1.f), vec3f(0, 0, 0.2)};

  std::vector<OSPPixelOp> pixelPipeline = {toneMap, colorTweak};
  OSPData pixelOpData =
      ospNewData(pixelPipeline.size(), OSP_OBJECT, pixelPipeline.data());
  ospCommit(pixelOpData);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->setPixelOps(pixelOpData);

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool pixelOpsUpdated = false;
    for (size_t i = 0; i < pixelPipeline.size(); ++i) {
      ImGui::PushID(i);

      bool enabled = enabledOps[i];
      if (ImGui::Checkbox("Enabled", &enabled)) {
        enabledOps[i]   = enabled;
        pixelOpsUpdated = true;
      }

      if (debugColors[i] != vec3f(-1.f)) {
        ImGui::Text("Debug Pixel Op #%lu", i);
        if (ImGui::ColorPicker3("Color", &debugColors[i].x)) {
          ospSetVec3f(pixelPipeline[i],
                   "addColor",
                   debugColors[i].x,
                   debugColors[i].y,
                   debugColors[i].z);
          pixelOpsUpdated = true;
          glfwOSPRayWindow->addObjectToCommit(pixelPipeline[i]);
        }
      } else {
        ImGui::Text("Tonemapper Pixel Op #%lu", i);
      }

      ImGui::PopID();
    }

    if (ImGui::Button("Add Debug Op")) {
      OSPPixelOp op = ospNewPixelOp("debug");
      ospSetVec3f(op, "addColor", 0.0, 0.0, 0.0);
      ospCommit(op);

      pixelPipeline.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(0));

      pixelOpsUpdated = true;
    }
    if (ImGui::Button("Add Tonemap Op")) {
      OSPPixelOp op = ospNewPixelOp("tonemapper");
      ospCommit(op);

      pixelPipeline.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(-1));

      pixelOpsUpdated = true;
    }

    if (!pixelPipeline.empty() && ImGui::Button("Remove Op")) {
      ospRelease(pixelPipeline.back());
      pixelPipeline.pop_back();
      enabledOps.pop_back();
      debugColors.pop_back();

      pixelOpsUpdated = true;
    }

    if (pixelOpsUpdated) {
      ospRelease(pixelOpData);
      std::vector<OSPPixelOp> enabled;
      for (size_t i = 0; i < pixelPipeline.size(); ++i) {
        if (enabledOps[i])
          enabled.push_back(pixelPipeline[i]);
      }

      pixelOpData = ospNewData(enabled.size(), OSP_OBJECT, enabled.data());
      ospCommit(pixelOpData);
      glfwOSPRayWindow->setPixelOps(pixelOpData);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(pixelOpData);

  for (auto &op : pixelPipeline)
    ospRelease(op);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
