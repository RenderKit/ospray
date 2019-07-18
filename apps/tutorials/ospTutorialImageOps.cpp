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
  initializeOSPRay(argc, argv, false);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  const bool denoiseModuleLoaded = ospLoadModule("denoiser") == OSP_NO_ERROR;

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
  //ospSetInt(renderer, "aoSamples", 0);

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);



  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly. We'll want albedo and normals for
  // the denoising op
  const uint32_t fbChannels = OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM
                              | OSP_FB_ALBEDO | OSP_FB_NORMAL;
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer,
          OSP_FB_RGBA32F, fbChannels));




  // Create a ImageOp pipeline to apply to the image
  OSPImageOp toneMap = ospNewImageOp("tonemapper");
  ospCommit(toneMap);

  // The colortweak pixel op will make the image more blue
  OSPImageOp colorTweak = ospNewImageOp("tile_debug");
  ospSetVec3f(colorTweak, "addColor", 0.0, 0.0, 0.2);
  ospCommit(colorTweak);
  std::vector<OSPObject> pixelOps = {toneMap, colorTweak};

  OSPImageOp frameOpTest = ospNewImageOp("frame_ssao");

  ospSetInt(frameOpTest, "ksize", 64);

  ospCommit(frameOpTest);
  std::vector<OSPObject> frameOps = {frameOpTest};

  // UI for the tweaking the pixel pipeline
  std::vector<bool> enabledOps   = {true, true, true};
  std::vector<vec3f> debugColors = {
    vec3f(-1.f), vec3f(0, 0, 0.2)
  };
  std::vector<std::string> frameOpNames = {"ssao"};

  std::vector<OSPObject> pixelPipeline = {toneMap, colorTweak, frameOpTest};
  OSPData pixelOpData =
      ospNewData(pixelPipeline.size(), OSP_IMAGE_OP, pixelPipeline.data());
  ospCommit(pixelOpData);


  glfwOSPRayWindow->setImageOps(pixelOpData);
  //glfwOSPRayWindow->setFrameOpsToUpdate(frameOps, frameOpNames);

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool pipelineUpdated = false;
    // Draw the UI for the Pixel Operations
    ImGui::Text("Image Operations");
    for (size_t i = 0; i < pixelOps.size(); ++i) {
      ImGui::PushID(i);

      bool enabled = enabledOps[i];
      if (ImGui::Checkbox("Enabled", &enabled)) {
        enabledOps[i]   = enabled;
        pipelineUpdated = true;
      }

      if (debugColors[i] != vec3f(-1.f)) {
        ImGui::Text("Debug Pixel Op #%lu", i);
        if (ImGui::ColorPicker3("Color", &debugColors[i].x)) {
          ospSetVec3f(pixelOps[i],
                      "addColor",
                      debugColors[i].x,
                      debugColors[i].y,
                      debugColors[i].z);
          pipelineUpdated = true;
          glfwOSPRayWindow->addObjectToCommit(pixelOps[i]);
        }
      } else {
        ImGui::Text("Tonemapper Pixel Op #%lu", i);
      }

      ImGui::PopID();
    }

    if (ImGui::Button("Add Debug PixelOp")) {
      OSPImageOp op = ospNewImageOp("tile_debug");
      ospSetVec3f(op, "addColor", 0.0, 0.0, 0.0);
      ospCommit(op);

      pixelOps.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(0));

      pipelineUpdated = true;
    }
    if (ImGui::Button("Add Tonemap ImageOp")) {
      OSPImageOp op = ospNewImageOp("tonemapper");
      ospCommit(op);

      pixelOps.push_back(op);
      enabledOps.push_back(true);
      debugColors.push_back(vec3f(-1));

      pipelineUpdated = true;
    }

    if (!pixelOps.empty() && ImGui::Button("Remove ImageOp")) {
      ospRelease(pixelOps.back());
      pixelOps.pop_back();
      enabledOps.pop_back();
      debugColors.pop_back();

      pipelineUpdated = true;
    }

    // Draw the UI for the Frame Operations
    ImGui::Separator();
    ImGui::Text("Frame Operations");
    for (size_t i = 0; i < frameOps.size(); ++i) {
      ImGui::PushID(i + pixelOps.size());

      ImGui::Text("Frame Op: %s", frameOpNames[i].c_str());
      bool enabled = enabledOps[i + pixelOps.size()];
      if (ImGui::Checkbox("Enabled", &enabled)) {
        enabledOps[i + pixelOps.size()]   = enabled;
        pipelineUpdated                   = true;
      }


      if(!frameOpNames[i].compare("ssao")){
        static float strength = 0.2f, radius = 0.3f, checkRadius = 1.f;
        if (ImGui::SliderFloat("strength", &strength, 0.f, 1.f) ) {
          ospSetFloat(frameOps[i], "strength", strength);
          pipelineUpdated                   = true;
          glfwOSPRayWindow->addObjectToCommit(frameOps[i]);
        }
        if (ImGui::SliderFloat("kernel radius", &radius, 0.f, 2.f) ) {
          ospSetFloat(frameOps[i], "radius", radius);
          pipelineUpdated                   = true;
          glfwOSPRayWindow->addObjectToCommit(frameOps[i]);
        }
        if (ImGui::SliderFloat("max ao distance", &checkRadius, 0.f, 2.f) ) {
          ospSetFloat(frameOps[i], "checkRadius", checkRadius);
          pipelineUpdated                   = true;
          glfwOSPRayWindow->addObjectToCommit(frameOps[i]);
        }
      }

      ImGui::PopID();
    }

    if (ImGui::Button("Add Debug FrameOp")) {
      OSPImageOp op = ospNewImageOp("frame_debug");
      ospCommit(op);

      frameOps.push_back(op);
      frameOpNames.push_back("frame_debug");
      enabledOps.push_back(true);

      pipelineUpdated = true;
    }
    if (ImGui::Button("Add Blur FrameOp")) {
      OSPImageOp op = ospNewImageOp("frame_blur");
      ospCommit(op);

      frameOps.push_back(op);
      frameOpNames.push_back("blur");
      enabledOps.push_back(true);

      pipelineUpdated = true;
    }
    if (ImGui::Button("Add Depth FrameOp")) {
      OSPImageOp op = ospNewImageOp("frame_depth");
      ospCommit(op);

      frameOps.push_back(op);
      frameOpNames.push_back("depth");
      enabledOps.push_back(true);

      pipelineUpdated = true;
    }
    if (ImGui::Button("Add SSAO FrameOp")) {
      OSPImageOp op = ospNewImageOp("frame_ssao");
      ospCommit(op);

      frameOps.push_back(op);
      frameOpNames.push_back("ssao");
      enabledOps.push_back(true);

      pipelineUpdated = true;
    }

    if (denoiseModuleLoaded && ImGui::Button("Add Denoise FrameOp")) {
      OSPImageOp op = ospNewImageOp("frame_denoise");
      ospCommit(op);

      frameOps.push_back(op);
      frameOpNames.push_back("denoise");
      enabledOps.push_back(true);

      pipelineUpdated = true;
    }

    if (!frameOps.empty() && ImGui::Button("Remove FrameOp")) {
      ospRelease(frameOps.back());
      frameOps.pop_back();
      enabledOps.pop_back();
      frameOpNames.pop_back();

      pipelineUpdated = true;
    }

    if (pipelineUpdated) {
      ospRelease(pixelOpData);
      std::vector<OSPObject> enabled;
      for (size_t i = 0; i < pixelOps.size(); ++i) {
        if (enabledOps[i])
          enabled.push_back(pixelOps[i]);
      }
      for (size_t i = 0; i < frameOps.size(); ++i) {
        if (enabledOps[i + pixelOps.size()])
          enabled.push_back(frameOps[i]);
      }

      pixelOpData = ospNewData(enabled.size(), OSP_IMAGE_OP, enabled.data());
      ospCommit(pixelOpData);
      glfwOSPRayWindow->setImageOps(pixelOpData);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(pixelOpData);

  for (auto &op : pixelOps)
    ospRelease(op);

  for (auto &op : frameOps)
    ospRelease(op);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
