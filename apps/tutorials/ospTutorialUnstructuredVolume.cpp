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

static std::string renderer_type = "scivis";

namespace {
  OSPVolumeInstance createVolumeWithTF(const char *volumeName,
                                       const char *tfName)
  {
    // create volume
    OSPTestingVolume tv = ospTestingNewVolume(volumeName);

    // create and set transfer function
    OSPTransferFunction tfn =
        ospTestingNewTransferFunction(tv.voxelRange, tfName);
    OSPVolumeInstance instance = ospNewVolumeInstance(tv.volume);

    ospSetObject(instance, "transferFunction", tfn);
    ospCommit(instance);
    ospRelease(tfn);
    ospRelease(tv.volume);

    // done
    return instance;
  }

  void setIsoValue(OSPGeometry geometry, float value)
  {
    // create and set a single iso value
    std::vector<float> isoValues = {value};
    OSPData isoValuesData =
        ospNewData(isoValues.size(), OSP_FLOAT, isoValues.data());
    ospSetData(geometry, "isovalues", isoValuesData);
    ospRelease(isoValuesData);
  }
}  // namespace

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

  // create all volume variances [sharedVertices][valuesPerCell]
  OSPVolumeInstance allVolumes[2][2];
  allVolumes[0][0] = createVolumeWithTF("simple_unstructured_volume_00", "jet");
  allVolumes[0][1] = createVolumeWithTF("simple_unstructured_volume_01", "jet");
  allVolumes[1][0] = createVolumeWithTF("simple_unstructured_volume_10", "jet");
  allVolumes[1][1] = createVolumeWithTF("simple_unstructured_volume_11", "jet");
  OSPVolumeInstance testVolume = allVolumes[0][0];

  // create iso geometry object and add it to the world
  OSPGeometry isoGeometry = ospNewGeometry("isosurfaces");

  // set initial iso value
  float isoValue = .2f;
  setIsoValue(isoGeometry, isoValue);

  // set volume object to create iso geometry
  ospSetObject(isoGeometry, "volume", testVolume);

  // create instance of the geometry
  OSPGeometryInstance instance = ospNewGeometryInstance(isoGeometry);

  // prepare material for iso geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospSet3f(material, "Ks", .2f, .2f, .2f);
  ospCommit(material);

  // assign material to the geometry
  ospSetMaterial(instance, material);

  // apply changes made
  ospCommit(isoGeometry);
  ospCommit(instance);
  ospCommit(world);

  // create OSPRay renderer
  OSPRenderer renderer = ospNewRenderer(renderer_type.c_str());

  // create and set lights
  OSPData lightsData = ospTestingNewLights("ambient_and_directional");
  ospSetData(renderer, "lights", lightsData);
  ospRelease(lightsData);

  // apply changes to the renderer
  ospCommit(renderer);

  // Scene updates //

  bool sharedVertices = false;
  bool valuesPerCell  = false;
  bool isoSurface     = false;

  auto updateScene = [&]() {
    ospRemoveParam(world, "volumes");
    ospRemoveParam(world, "geometries");

    // remove current volume
    ospRemoveParam(isoGeometry, "volume");

    // set a new one
    testVolume = allVolumes[sharedVertices ? 1 : 0][valuesPerCell ? 1 : 0];

    // attach the new volume
    ospSetObject(isoGeometry, "volume", testVolume);

    if (isoSurface) {
      OSPData geomInsts = ospNewData(1, OSP_OBJECT, &instance);
      ospSetObject(world, "geometries", geomInsts);
      ospRelease(geomInsts);
    } else {
      OSPData volInsts = ospNewData(1, OSP_OBJECT, &testVolume);
      ospSetObject(world, "volumes", volInsts);
      ospRelease(volInsts);
    }
  };

  updateScene();
  ospCommit(world);

  // create a GLFW OSPRay window: this object will create and manage the OSPRay
  // frame buffer and camera directly
  auto glfwOSPRayWindow =
      std::unique_ptr<GLFWOSPRayWindow>(new GLFWOSPRayWindow(
          vec2i{1024, 768}, box3f(vec3f(-1.f), vec3f(1.f)), world, renderer));

  glfwOSPRayWindow->registerImGuiCallback([&]() {
    bool updateWorld = false;

    if ((ImGui::Checkbox("shared vertices", &sharedVertices)) ||
        (ImGui::Checkbox("per-cell values", &valuesPerCell)))
      updateWorld = true;

    if (ImGui::Checkbox("isosurface", &isoSurface))
      updateWorld = true;

    if (isoSurface) {
      if (ImGui::SliderFloat("iso value", &isoValue, 0.f, 1.f)) {
        // update iso value
        setIsoValue(isoGeometry, isoValue);
        glfwOSPRayWindow->addObjectToCommit(isoGeometry);
        glfwOSPRayWindow->addObjectToCommit(instance);
        glfwOSPRayWindow->addObjectToCommit(world);
      }

      static float isoOpacity = 1.f;
      if (ImGui::SliderFloat("iso opacity", &isoOpacity, 0.f, 1.f)) {
        ospSet1f(material, "d", isoOpacity);
        glfwOSPRayWindow->addObjectToCommit(material);
      }
    }

    if (updateWorld) {
      updateScene();
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
  ospRelease(instance);
  ospRelease(material);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
