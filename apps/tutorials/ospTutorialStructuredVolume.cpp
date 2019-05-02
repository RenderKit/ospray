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

#include "ospcommon/library.h"
#include "ospcommon/range.h"
#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

static std::string renderer_type = "scivis";

static void setIsoValue(OSPGeometry geometry, float value)
{
  // create and set a single iso value
  OSPData isoValuesData = ospNewData(1, OSP_FLOAT, &value);
  ospSetData(geometry, "isovalues", isoValuesData);
  ospRelease(isoValuesData);
}

static void setSlice(OSPGeometry geometry, float value)
{
  vec4f plane(-1.f, 0.f, 0.f, value);
  OSPData planesData = ospNewData(1, OSP_FLOAT4, &plane);
  ospSetData(geometry, "planes", planesData);
  ospRelease(planesData);
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospGetCurrentDevice(), [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the world which will contain all of our geometries / volumes
  OSPWorld world = ospNewWorld();

  // Create volume

#if 0
  OSPTestingVolume test_data = ospTestingNewVolume("simple_structured_volume");
#else
  OSPTestingVolume test_data = ospTestingNewVolume("gravity_spheres_volume");
#endif

  auto volume     = test_data.volume;
  auto voxelRange = (range1f &)test_data.voxelRange;

  OSPTransferFunction tfn =
      ospTestingNewTransferFunction(test_data.voxelRange, "jet");

  ospSetObject(volume, "transferFunction", tfn);
  ospCommit(volume);

  auto instance = ospNewVolumeInstance(volume);
  ospSetObject(instance, "transferFunction", tfn);
  ospCommit(instance);

  //ospAddVolume(world, volume);
  ospAddVolumeInstance(world, instance);
  ospRelease(tfn);

  // Create isosurface geometry //

  // create iso geometry object and add it to the world
  OSPGeometry isoGeometry = ospNewGeometry("isosurfaces");

  // set initial iso value
  float isoValue = voxelRange.center() / 2.f;
  setIsoValue(isoGeometry, isoValue);

  // set volume object to create iso geometry
  ospSetObject(isoGeometry, "volume", volume);

  // create isoInstance of the geometry
  OSPGeometryInstance isoInstance = ospNewGeometryInstance(isoGeometry);

  // prepare material for iso geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospSet3f(material, "Ks", .2f, .2f, .2f);
  ospCommit(material);

  // assign material to the geometry
  ospSetMaterial(isoInstance, material);

  // Create slices geometry //

  OSPGeometry sliceGeometry = ospNewGeometry("slices");
  ospSetObject(sliceGeometry, "volume", volume);
  float sliceValue = 0.f;
  setSlice(sliceGeometry, sliceValue);
  OSPGeometryInstance sliceInstance = ospNewGeometryInstance(sliceGeometry);

  // apply changes made
  ospCommit(isoGeometry);
  ospCommit(isoInstance);
  ospCommit(sliceGeometry);
  ospCommit(sliceInstance);
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  OSPData lightsData = ospTestingNewLights("ambient_only");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospSet1i(renderer, "aoSamples", 1);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow = std::unique_ptr<GLFWOSPRayWindow>(
      new GLFWOSPRayWindow(vec2i{1024, 768},
                           reinterpret_cast<box3f &>(test_data.bounds),
                           world,
                           renderer));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    static int aoSamples = 1;
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64)) {
      ospSet1i(renderer, "aoSamples", aoSamples);
      glfwOSPRayWindow->addObjectToCommit(renderer);
    }

    ImGui::NewLine();
    ImGui::Text("Show:");

    bool commitWorld = false;

    static bool showVolume = true;
    if (ImGui::Checkbox("volume", &showVolume)) {
      commitWorld = true;
      if (showVolume)
        ospAddVolumeInstance(world, instance);
      else
        ospRemoveVolumeInstance(world, instance);
    }

    static bool showIsoSurface = false;
    if (ImGui::Checkbox("isosurface", &showIsoSurface)) {
      commitWorld = true;
      if (showIsoSurface)
        ospAddGeometryInstance(world, isoInstance);
      else
        ospRemoveGeometryInstance(world, isoInstance);
    }

    static bool showSlice = false;
    if (ImGui::Checkbox("slice", &showSlice)) {
      commitWorld = true;
      if (showSlice)
        ospAddGeometryInstance(world, sliceInstance);
      else
        ospRemoveGeometryInstance(world, sliceInstance);
    }

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    if (ImGui::SliderFloat(
            "iso value", &isoValue, voxelRange.lower, voxelRange.upper)) {
      setIsoValue(isoGeometry, isoValue);
      glfwOSPRayWindow->addObjectToCommit(isoGeometry);
      glfwOSPRayWindow->addObjectToCommit(isoInstance);
    }

    static float isoOpacity = 1.f;
    if (ImGui::SliderFloat("iso opacity", &isoOpacity, 0.f, 1.f)) {
      ospSet1f(material, "d", isoOpacity);
      glfwOSPRayWindow->addObjectToCommit(material);
    }

    if (ImGui::SliderFloat("slice position", &sliceValue, -1.f, 1.f)) {
      commitWorld = true;
      setSlice(sliceGeometry, sliceValue);
      glfwOSPRayWindow->addObjectToCommit(sliceGeometry);
      glfwOSPRayWindow->addObjectToCommit(sliceInstance);
    }

    if (commitWorld)
      glfwOSPRayWindow->addObjectToCommit(world);
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(volume);
  ospRelease(instance);
  ospRelease(isoInstance);
  ospRelease(isoGeometry);
  ospRelease(sliceGeometry);
  ospRelease(sliceInstance);
  ospRelease(material);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
