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

#include <vector>
#include "GLFWOSPRayWindow.h"

#include "ospcommon/library.h"
#include "ospray_testing.h"

#include <imgui.h>

using namespace ospcommon;

namespace {
  OSPVolume createVolumeWithTF(const char* volumeName, const char* tfName)
  {
    // create volume
    OSPTestingVolume tv = ospTestingNewVolume(volumeName);

    // create and set transfer function
    OSPTransferFunction tfn =
      ospTestingNewTransferFunction(tv.voxelRange, tfName);
    ospSetObject(tv.volume, "transferFunction", tfn);
    ospCommit(tv.volume);
    ospRelease(tfn);

    // done
    return tv.volume;
  }

  void setIsoValue(OSPGeometry geometry, float value)
  {
    std::vector<float> isoValues = { value };
    OSPData isoValuesData =
      ospNewData(isoValues.size(), OSP_FLOAT, isoValues.data());
    ospSetData(geometry, "isovalues", isoValuesData);
    ospRelease(isoValuesData);
  }
}

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    return initError;

  // we must load the testing library explicitly on Windows to look up
  // object creation functions
  loadLibrary("ospray_testing");

  // get OSPRay device
  OSPDevice ospDevice = ospGetCurrentDevice();
  if (!ospDevice)
    return -1;

  // set an error callback to catch any OSPRay errors and exit the application
  ospDeviceSetErrorFunc(
      ospDevice, [](OSPError error, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
        exit(error);
      });

  // create the world which will contain all of our geometries / volumes
  OSPWorld world = ospNewWorld();

  // add in generated volume and transfer function
  OSPVolume allVolumes[2][2];
  allVolumes[0][0] = createVolumeWithTF("simple_unstructured_volume_00", "jet");
  allVolumes[0][1] = createVolumeWithTF("simple_unstructured_volume_01", "jet");
  allVolumes[1][0] = createVolumeWithTF("simple_unstructured_volume_10", "jet");
  allVolumes[1][1] = createVolumeWithTF("simple_unstructured_volume_11", "jet");
  OSPVolume testVolume = allVolumes[0][0];

  // add volume to the world
  ospAddVolume(world, testVolume);

  // create iso geometry object and add it to the world
  OSPGeometry isoGeometry = ospNewGeometry("isosurfaces");

  // set initial iso values
  float isoValue = .2f;
  setIsoValue(isoGeometry, isoValue);

  // set volume object to create iso geometry
  ospSetObject(isoGeometry, "volume", testVolume);

  // prepare material for iso geometry
  OSPMaterial material = ospNewMaterial("scivis", "OBJMaterial");
  ospSet3f(material, "Ks", 0.5f, 0.5f, 0.5f);
  ospSet1f(material, "Ns", 100.0f);
  ospCommit(material);

  // assign material to geometry
  ospSetMaterial(isoGeometry, material);
  ospRelease(material);

  // apply changes to iso geometry object
  ospCommit(isoGeometry);

  // commit the world
  ospCommit(world);

  // Create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer("scivis");

  OSPData lightsData = ospTestingNewLights("ambient_and_directional");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  ospCommit(renderer);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    static int spp = 1;
    if (ImGui::SliderInt("spp", &spp, 1, 64)) {
      ospSet1i(renderer, "spp", spp);
      ospCommit(renderer);
    }
    static bool sharedVertices = false;
    static bool valuesPerCell = false;
    static bool isoSurface = false;
    if ((ImGui::Checkbox("shared vertices", &sharedVertices)) ||
        (ImGui::Checkbox("per-cell values", &valuesPerCell))) {

      // remove current volume
      ospRemoveParam(isoGeometry, "volume");
      if (!isoSurface)
        ospRemoveVolume(world, testVolume);

      // set a new one
      testVolume =
        allVolumes[sharedVertices ? 1 : 0][valuesPerCell ? 1 : 0];

      // attach the new volume
      ospSetObject(isoGeometry, "volume", testVolume);
      if (!isoSurface)
        ospAddVolume(world, testVolume);
      glfwOSPRayWindow->addObjectToCommit(isoGeometry);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
    if (ImGui::Checkbox("isosurface", &isoSurface)) {
      if (isoSurface) {
        // replace volume with iso geometry
        ospRemoveVolume(world, testVolume);
        ospAddGeometry(world, isoGeometry);
        glfwOSPRayWindow->addObjectToCommit(world);
      } else {
        // replace iso geometry with volume
        ospAddVolume(world, testVolume);
        ospRemoveGeometry(world, isoGeometry);
        glfwOSPRayWindow->addObjectToCommit(world);
      }
    }
    if ((isoSurface) &&
        (ImGui::SliderFloat("iso value", &isoValue, 0.f, 1.f))) {
      // update iso value
      setIsoValue(isoGeometry, isoValue);
      glfwOSPRayWindow->addObjectToCommit(isoGeometry);
      glfwOSPRayWindow->addObjectToCommit(world);
    }
  });

  // start the GLFW main loop, which will continuously render
  glfwOSPRayWindow->mainLoop();

  // cleanup remaining objects
  ospRelease(allVolumes[0][0]);
  ospRelease(allVolumes[0][1]);
  ospRelease(allVolumes[1][0]);
  ospRelease(allVolumes[1][1]);
  ospRelease(isoGeometry);
  ospRelease(world);
  ospRelease(renderer);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
