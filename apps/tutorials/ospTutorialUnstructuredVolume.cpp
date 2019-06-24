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

#include "tutorial_util.h"

#include <imgui.h>

using namespace ospcommon;

static std::string renderer_type = "scivis";

namespace {
  OSPVolumetricModel createVolumeWithTF(const char *volumeName,
                                        const char *tfName)
  {
    // create volume
    OSPTestingVolume tv = ospTestingNewVolume(volumeName);

    // create and set transfer function
    OSPTransferFunction tfn =
        ospTestingNewTransferFunction(tv.voxelRange, tfName);
    OSPVolumetricModel model = ospNewVolumetricModel(tv.volume);

    ospSetObject(model, "transferFunction", tfn);
    ospCommit(model);
    ospRelease(tfn);
    ospRelease(tv.volume);

    // done
    return model;
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
  initializeOSPRay(argc, argv);

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-r" || arg == "--renderer")
      renderer_type = argv[++i];
  }

  // create the world which will contain all of our geometries / volumes
  OSPWorld world   = ospNewWorld();
  OSPInstance inst = ospNewInstance();

  OSPData instances = ospNewData(1, OSP_OBJECT, &inst);
  ospSetData(world, "instances", instances);
  ospRelease(instances);

  // create all volume variances [sharedVertices][valuesPerCell]
  OSPVolumetricModel allVolumes[2][2];
  allVolumes[0][0] = createVolumeWithTF("simple_unstructured_volume_00", "jet");
  allVolumes[0][1] = createVolumeWithTF("simple_unstructured_volume_01", "jet");
  allVolumes[1][0] = createVolumeWithTF("simple_unstructured_volume_10", "jet");
  allVolumes[1][1] = createVolumeWithTF("simple_unstructured_volume_11", "jet");
  OSPVolumetricModel testVolume = allVolumes[0][0];

  // create iso geometry object and add it to the world
  OSPGeometry isoGeometry = ospNewGeometry("isosurfaces");

  // set initial iso value
  float isoValue = .2f;
  setIsoValue(isoGeometry, isoValue);

  // set volume object to create iso geometry
  ospSetObject(isoGeometry, "volume", testVolume);

  // create instance of the geometry
  OSPGeometricModel model = ospNewGeometricModel(isoGeometry);

  // prepare material for iso geometry
  OSPMaterial material = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
  ospSetVec3f(material, "Ks", .2f, .2f, .2f);
  ospCommit(material);

  // assign material to the geometry
  ospSetObject(model, "material", material);

  // apply changes made
  ospCommit(isoGeometry);
  ospCommit(model);

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
    ospRemoveParam(inst, "volumes");
    ospRemoveParam(inst, "geometries");

    // remove current volume
    ospRemoveParam(isoGeometry, "volume");

    // set a new one
    testVolume = allVolumes[sharedVertices ? 1 : 0][valuesPerCell ? 1 : 0];

    // attach the new volume
    ospSetObject(isoGeometry, "volume", testVolume);

    if (isoSurface) {
      OSPData geomModels = ospNewData(1, OSP_OBJECT, &model);
      ospSetObject(inst, "geometries", geomModels);
      ospRelease(geomModels);
    } else {
      OSPData volModels = ospNewData(1, OSP_OBJECT, &testVolume);
      ospSetObject(inst, "volumes", volModels);
      ospRelease(volModels);
    }
  };

  updateScene();
  ospCommit(inst);
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
        glfwOSPRayWindow->addObjectToCommit(model);
        glfwOSPRayWindow->addObjectToCommit(inst);
        glfwOSPRayWindow->addObjectToCommit(world);
      }

      static float isoOpacity = 1.f;
      if (ImGui::SliderFloat("iso opacity", &isoOpacity, 0.f, 1.f)) {
        ospSetFloat(material, "d", isoOpacity);
        glfwOSPRayWindow->addObjectToCommit(material);
      }
    }

    if (updateWorld) {
      updateScene();
      glfwOSPRayWindow->addObjectToCommit(isoGeometry);
      glfwOSPRayWindow->addObjectToCommit(inst);
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
  ospRelease(model);
  ospRelease(inst);
  ospRelease(material);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
