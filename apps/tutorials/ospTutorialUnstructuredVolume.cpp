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
    ospSetObject(geometry, "isovalue", isoValuesData);
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
  OSPWorld world = ospNewWorld();
  OSPGroup group = ospNewGroup();

  OSPInstance instance = ospNewInstance(group);
  ospCommit(instance);

  OSPData instances = ospNewData(1, OSP_INSTANCE, &instance);
  ospSetObject(world, "instance", instances);
  ospRelease(instances);
  ospRelease(instance);

  // create and set lights
  OSPData lightsData = ospTestingNewLights("ambient_and_directional");
  ospSetObject(world, "light", lightsData);
  ospRelease(lightsData);

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
  // Scene updates //

  bool sharedVertices = false;
  bool valuesPerCell  = false;
  bool isoSurface     = false;

  auto updateScene = [&]() {
    ospRemoveParam(group, "volume");
    ospRemoveParam(group, "geometry");

    // remove current volume
    ospRemoveParam(isoGeometry, "volume");

    // set a new one
    testVolume = allVolumes[sharedVertices ? 1 : 0][valuesPerCell ? 1 : 0];

    // attach the new volume
    ospSetObject(isoGeometry, "volume", testVolume);

    if (isoSurface) {
      OSPData geomModels = ospNewData(1, OSP_GEOMETRIC_MODEL, &model);
      ospSetObject(group, "geometry", geomModels);
      ospRelease(geomModels);
    } else {
      OSPData volModels = ospNewData(1, OSP_VOLUMETRIC_MODEL, &testVolume);
      ospSetObject(group, "volume", volModels);
      ospRelease(volModels);
    }
  };

  updateScene();
  ospCommit(group);
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
        glfwOSPRayWindow->addObjectToCommit(group);
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
      glfwOSPRayWindow->addObjectToCommit(group);
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
  ospRelease(group);
  ospRelease(material);

  // cleanly shut OSPRay down
  ospShutdown();

  return 0;
}
